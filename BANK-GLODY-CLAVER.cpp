#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <limits>
#include <cmath>
#include <sstream> 
#include <cstdio> 

// Gestion automatique des fonctions d'interception clavier selon l'OS
#ifdef _WIN32
#include <conio.h>
#include <windows.h> 
#else
#include <termios.h>
#include <unistd.h>
#endif

// --- CONSTANTES ET LIMITES GENERALES DE L'APPLICATION ---
const int MAX_TRANSACTIONS = 20; // Historique maximum des opérations par client
const int MAX_CLIENTS = 15;      // Capacité maximale du registre des comptes clients
const int FILET_FOREX_SIZE = 30; // Nombre de points affichés sur les graphiques boursiers

// --- STRUCTURE DE RELEVÉ ET HISTORIQUE D'OPÉRATION ---
struct Transaction {
    std::string type;             // Nature de l'opération (Depot, Retrait, Virement...)
    double montant;               // Somme d'argent mise en jeu
    std::string devise;           // Monnaie utilisée (FC, USD, EUR, CNY, GBP, RTC)
    std::string paysDestination;  // Pays qui reçoit l'argent de l'opération
    std::string nomExpediteur;    // Identifiant de la personne qui a fait l'opération
    bool suspecte;                // Indicateur de blanchiment d'argent (Alerte TRACFIN)
};

// --- STRUCTURE DE CARTE BANCAIRE VIRTUELLE ---
struct CarteBancaire {
    std::string numero;           // Numéro unique à 16 chiffres généré par le système
    std::string typeCode;         // Modèle de la carte : Classic, Gold ou Platinum
    double plafondHebdo;          // Limite maximale de dépenses autorisée en USD
    double depenseCouranteHebdo;  // Total des dépenses déjà faites cette semaine en USD
    bool active;                  // Statut de la carte (True = Active / False = Bloquée)
};

// --- STRUCTURE MAITRESSE DU COMPTE CLIENT ---
struct Client {
    std::string identifiant;      // ID de connexion unique (ex: Jean123)
    std::string codeSecret;       // Code secret d'authentification à 4 chiffres
    std::string sexe;             // Genre de l'utilisateur (M/F)
    std::string pays;             // Pays de résidence (RDC, France, Canada, Chine, Royaume-Uni)
    
    // Compartiments des Soldes de Comptes Courants
    double soldeCourantFC;        // Franc Congolais (RDC)
    double soldeCourantUSD;       // Dollar Américain (Monnaie pivot)
    double soldeCourantEUR;       // Euro (Zone Europe)
    double soldeCourantCNY;       // Yuan Renminbi (Chine)
    double soldeCourantGBP;       // Livre Sterling (Royaume-Uni)

    // Compartiments des Soldes de Comptes Épargne (Placements rémunérés)
    double soldeEpargneFC;
    double soldeEpargneUSD;
    double soldeEpargneEUR;
    double soldeEpargneCNY;
    double soldeEpargneGBP;
    
    // Paramètres du module de Crédit (Dettes actives)
    double dettePretUSD;          // Encours total des emprunts contractés en USD
    int joursDepuisPret;          // Nombre de jours d'ancienneté du crédit sans remboursement
    
    // Paramètres du module de Trading (Courtage sur marge et levier)
    double capitalLevierUSD;      // Collatéral de marge déposé par le client en USD
    double prixEntreeLevierRTC;   // Cours d'achat initial du RetroCoin lors du levier
    double taillePositionRTC;     // Volume virtuel de tokens RetroCoins piloté avec le levier

    // Historique, sécurité et métriques analytiques
    Transaction historique[MAX_TRANSACTIONS];
    int nbTransactions;           // Nombre effectif d'opérations dans l'historique
    int totalTransfertsEmis;      // Compteur de virements internationaux émis (Indice TRACFIN)
    int messagesNonLus;           // Compteur de notifications push dans la messagerie
    int scoreRisque;              // Note de suspicion globale du client (0 à 100%)
    bool estGele;                 // Compte bloqué de force par la sécurité anti-fraude
    bool alerteBlanchiment;       // Drapeau d'infraction financière levé (TRACFIN)
    double retroCoins;            // Solde de crypto-actifs RetroCoins en portefeuille Spot
    long long dernierAccesTimestamp; // Horodatage UNIX de la dernière connexion pour l'inflation
    
    // Paramètres de l'Automate de trading (Ordres à seuil déclenché)
    double declencheurAchatRTC;   // Prix cible sous lequel l'automate doit s'activer (USD)
    double volumeAchatRTC;        // Quantité de RetroCoins à acquérir de force
    
    CarteBancaire carte;          // Composant carte virtuelle associé
    
    // Gestion du désordre : Compteurs physiques de fichiers générés par type pour ce client
    int nbFichiersDepots;         // Nombre de reçus créés dans /depots/ (ex: depot_1.txt, etc.)
    int nbFichiersRetraits;       // Nombre de reçus créés dans /retraits/
    int nbFichiersVirements;      // Nombre de reçus créés dans /virements/
};

// --- SYSTEME DE CLASSEMENT AUTOMATIQUE DES DOSSIERS ---

// Crée un sous-dossier de manière sécurisée sous l'environnement Windows
void creerDossier(const std::string& chemin) {
#ifdef _WIN32
    CreateDirectoryA(chemin.c_str(), NULL);
#endif
}

// Génère automatiquement l'arborescence complète et cloisonnée pour un client spécifique
void creerArborescenceClient(const std::string& id) {
    creerDossier("clients");                      // Crée le dossier racine des utilisateurs
    creerDossier("clients/" + id);                // Crée le dossier au nom du client
    creerDossier("clients/" + id + "/depots");    // Dossier pour classer les reçus de dépôts
    creerDossier("clients/" + id + "/retraits");  // Dossier pour classer les reçus de retraits
    creerDossier("clients/" + id + "/virements"); // Dossier pour classer les reçus de virements
}

// --- LOGIQUE D'AFFICHAGE ET NETTOYAGE DES ÉCRANS ---

// Vide le tampon de saisie pour empêcher le terminal de planter si l'utilisateur se trompe de touche
void viderEntree() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Configure la console en Cyan brillant (Thème par défaut de BANK-GLODY-CLAVER)
void ecranBleu() {
#ifdef _WIN32
    system("color 0B"); system("cls");
#else
    std::cout << "\033[1;34m\033[2J\033[1;1H";
#endif
}

// Configure la console en Vert brillant (Thème Moteur de Trading / Clavier)
void ecranVert() {
#ifdef _WIN32
    system("color 0A"); system("cls");
#else
    std::cout << "\033[1;32m\033[2J\033[1;1H";
#endif
}

// Configure la console en Rouge (Thème d'Alertes Fraude ou Margin Calls)
void ecranRouge() {
#ifdef _WIN32
    system("color 0C"); system("cls");
#else
    std::cout << "\033[1;31m\033[2J\033[1;1H";
#endif
}

// Configure la console en Jaune (Thème de Surveillance TRACFIN ou Crises)
void ecranJaune() {
#ifdef _WIN32
    system("color 0E"); system("cls");
#else
    std::cout << "\033[1;33m\033[2J\033[1;1H";
#endif
}

// Suspend l'activité du programme durant un certain nombre de millisecondes
void fairePause(int millisecondes) {
#ifdef _WIN32
    Sleep(millisecondes);
#else
    usleep(millisecondes * 1000);
#endif
}

// --- MOTEURS D'ANIMATION ET CHARGEMENT GRAPHIQUE ---

// Affiche une barre de progression animée au démarrage de la banque
void effetChargementInitial() {
    ecranBleu();
    std::cout << "\n\n\n\n\n\n";
    std::cout << "               =============================================\n";
    std::cout << "                INITIALISATION DU SYSTEME BANK-GLODY-CLAVER \n";
    std::cout << "               =============================================\n\n";
    std::cout << "               Chargement des modules securite ";
    
    for (int i = 0; i < 6; ++i) {
        std::cout << ".";
        std::cout.flush();
        fairePause(250);
    }
    std::cout << "\n\n               [";
    for (int i = 0; i < 20; ++i) {
        std::cout << "#";
        std::cout.flush();
        fairePause(60); // Vitesse de la barre de chargement
    }
    std::cout << "] 100%\n\n";
    std::cout << "               -> Infrastructure prete. Connexion au terminal...";
    fairePause(900);
}

// Affiche un écran de défilement de code binaire boursier ultra-ralenti
void effetMatrix() {
    ecranVert();
    std::cout << " +--------------------------------------------------------+\n"
              << " | [BANK-GLODY-CLAVER NETWORK] ROUTAGE INTERBANCAIRE...   |\n"
              << " | ENCRYPTION DES NOEUDS COMPENSATOIRES SWIFT...          |\n"
              << " +--------------------------------------------------------+\n\n";
    fairePause(400); 
    for (int i = 0; i < 18; ++i) { 
        std::cout << "   ";
        for (int j = 0; j < 12; ++j) { 
            std::cout << (std::rand() % 2) << (std::rand() % 2) << "  "; 
        }
        std::cout << '\n'; 
        fairePause(120); // Temps augmenté pour un meilleur rendu visuel
    }
    std::cout << "\n -> [SUCCES] Signal monetaire approuve par les validateurs distants.\n";
    fairePause(800); 
    ecranBleu();
}

// Intercepte les saisies clavier pour afficher des '*' à l'écran (Masquage du code secret)
std::string lireCodeEtoile() {
    std::string code = "";
#ifdef _WIN32
    char ch;
    while (true) {
        ch = (char)_getch();
        if (ch == 13) break; // Touche Entrée
        else if (ch == 8) { // Touche Effacer (Retour arrière)
            if (!code.empty()) { 
                code.erase(code.length() - 1, 1); 
                std::cout << "\b \b"; 
            } 
        }
        else if (ch >= 32 && ch <= 126) { 
            code += ch; 
            std::cout << '*'; 
        }
    }
#else
    termios ancien, nouveau; tcgetattr(STDIN_FILENO, &ancien); nouveau = ancien; nouveau.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &nouveau); std::getline(std::cin >> std::ws, code);
    tcsetattr(STDIN_FILENO, TCSANOW, &ancien); std::cout << std::string(code.size(), '*');
#endif
    std::cout << '\n'; return code;
}

// Force l'utilisateur à appuyer sur Entrée pour valider la lecture d'un message ou reçu
void attendreRetour() {
    std::cout << "\n [Appuyez sur Entree pour revenir au menu precedent]... ";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); std::cin.get();
}

// --- ENSEMBLE DES CADRES GRAPHIQUES DE NAVIGATION (ASCII) ---

// Menu d'accueil public general de la banque
void menuAccueil() {
    ecranBleu();
    std::cout << " +-------------------------------------------------------+\n"
              << " |                 INFRASTRUCTURE FINANCIERE             |\n"
              << " |                   BANK-GLODY-CLAVER                   |\n"
              << " +-------------------------------------------------------+\n"
              << " |  1. Ouvrir une session client securisee (Guichet)     |\n"
              << " |  2. Acceder au Terminal d'Administration (Masque)     |\n"
              << " |  3. Eteindre le terminal de banque                    |\n"
              << " +-------------------------------------------------------+\n"
              << "  Faites votre choix (1-3) : ";
}

// Panneau d'administration maître de l'infrastructure
void menuAdmin() {
    ecranBleu();
    std::cout << " +-------------------------------------------------------+\n"
              << " |             PANNEAU CONFIGURATION ADMIN PREMIUM       |\n"
              << " |                   BANK-GLODY-CLAVER                   |\n"
              << " +-------------------------------------------------------+\n"
              << " |  1. Creer un nouveau compte client                    |\n"
              << " |  2. Classement des clients par richesse globale (USD) |\n"
              << " |  3. Consulter l'historique graphique des devises      |\n"
              << " |  4. Voir le volume total des flux de virement emis    |\n"
              << " |  5. [TRACFIN] Surveillance des risques & blanchiment  |\n"
              << " |  6. Commuter la maintenance globale du systeme        |\n"
              << " |  7. Graphique : Histogramme analytique des Dettes     |\n"
              << " |  8. Graphique : Analyse spectrale monetaire           |\n"
              << " |  9. Decisions anti-fraude & Lever le gel d'un compte  |\n"
              << " | 10. [CRISE] Executer une Taxe sur la fortune (2%)     |\n"
              << " | 11. [TAUX] Modifier la puissance des taux de change   |\n"
              << " | 12. Se deconnecter de l'infrastructure Admin          |\n"
              << " +-------------------------------------------------------+\n"
              << "  Faites votre choix (1-12) : ";
}

// Portail d'operations securisees destine au client authentifie
void menuClient(const Client* c, double coursCrypto) {
    ecranBleu();
    std::cout << " +-------------------------------------------------------+\n"
              << " |           PORTAIL CLIENT RETRO MULTIDEVISES           |\n"
              << " |                   BANK-GLODY-CLAVER                   |\n"
              << " +-------------------------------------------------------+\n"
              << " |  1. Consulter mes Soldes, Dettes actives & Indices    |\n"
              << " |  2. Deposer des fonds (Genere recu .txt classe)       |\n"
              << " |  3. Retirer des devises (Plafond Carte Actif)         |\n"
              << " |  4. Transfert de fonds Inter-Comptes (Courant/Epargne)|\n"
              << " |  5. Passer par le Bureau de Change instantane         |\n"
              << " |  6. Initier un virement international (TRACFIN)       |\n"
              << " |  7. Gestion des demandes et remboursements de Pret    |\n"
              << " |  8. Boite de reception - Messagerie bancaire [" << c->messagesNonLus << "]       |\n"
              << " |  9. Visualiser ma courbe de Tendance Financiere       |\n"
              << " | 10. [BROKER] Trading RetroCoin (Cours: " << coursCrypto << " USD)     |\n"
              << " | 11. [MARGE] Speculation Crypto avec Effet de Levier   |\n"
              << " | 12. [ANALYSE] Graphique Solde vs Endettement          |\n"
              << " | 13. [AUTOMATE] Programmer un ordre d'achat limite     |\n"
              << " | 14. [CARTE] Gerer ma Carte Bancaire Virtuelle         |\n"
              << " | 15. Exporter un releve complet global (.txt)          |\n"
              << " | 16. Clore la session client securisee                 |\n"
              << " +-------------------------------------------------------+\n"
              << "  Utilisateur: " << c->identifiant << (c->estGele ? " [STATUT: GELE]" : " [STATUT: ACTIF]") << "\n"
              << " +-------------------------------------------------------+\n"
              << "  Faites votre choix (1-16) : ";
}

// Écrit une ligne horodatée et explicite dans le fichier d'audit général logé dans banque/
void ecrireAudit(const std::string& message) {
    creerDossier("banque"); // Sécurité : s'assure que le dossier existe
    std::ofstream fichier("banque/banque_audit.log", std::ios::app); 
    if (!fichier.is_open()) return;
    
    std::time_t t = std::time(NULL); 
    std::string strTime(ctime(&t));
    if(!strTime.empty() && strTime[strTime.length()-1] == '\n') {
        strTime.erase(strTime.length()-1);
    }
    
    fichier << "[" << strTime << "] " << message << "\n"; 
    fichier.close();
}

// Enregistre l'ensemble des données dans l'historique d'un utilisateur
void ajouterTransaction(Client& client, const std::string& type, double montant, const std::string& devise, const std::string& pays, const std::string& expediteur, bool suspecte) {
    Transaction t; 
    t.type = type; 
    t.montant = montant; 
    t.devise = devise; 
    t.paysDestination = pays; 
    t.nomExpediteur = expediteur; 
    t.suspecte = suspecte;
    
    if (client.nbTransactions < MAX_TRANSACTIONS) { 
        client.historique[client.nbTransactions++] = t; 
    } 
    else {
        // Décale l'historique pour supprimer la plus ancienne opération (File d'attente FIFO)
        for (int i = 1; i < MAX_TRANSACTIONS; ++i) {
            client.historique[i - 1] = client.historique[i];
        }
        client.historique[MAX_TRANSACTIONS - 1] = t;
    }
}

// Sauvegarde l'intégralité du registre bancaire de manière ordonnée dans banque/
void sauvegarderDonnees(const Client list[], int nbC) {
    creerDossier("banque");
    std::ofstream fichier("banque/banque_sauvegarde.txt"); 
    if (!fichier.is_open()) return;
    
    fichier << nbC << '\n';
    for (int i = 0; i < nbC; ++i) {
        fichier << list[i].identifiant << ' ' << list[i].codeSecret << ' ' << list[i].sexe << ' ' << list[i].pays << ' '
                << list[i].soldeCourantFC << ' ' << list[i].soldeCourantUSD << ' ' << list[i].soldeCourantEUR << ' '
                << list[i].soldeCourantCNY << ' ' << list[i].soldeCourantGBP << ' '
                << list[i].soldeEpargneFC << ' ' << list[i].soldeEpargneUSD << ' ' << list[i].soldeEpargneEUR << ' '
                << list[i].soldeEpargneCNY << ' ' << list[i].soldeEpargneGBP << ' '
                << list[i].dettePretUSD << ' ' << list[i].joursDepuisPret << ' ' << list[i].capitalLevierUSD << ' ' 
                << list[i].prixEntreeLevierRTC << ' ' << list[i].taillePositionRTC << ' '
                << list[i].totalTransfertsEmis << ' ' << list[i].messagesNonLus << ' ' << list[i].nbTransactions << ' ' 
                << list[i].scoreRisque << ' ' << list[i].estGele << ' ' << list[i].alerteBlanchiment << ' ' 
                << list[i].retroCoins << ' ' << list[i].dernierAccesTimestamp << ' ' 
                << list[i].declencheurAchatRTC << ' ' << list[i].volumeAchatRTC << ' '
                << list[i].carte.numero << ' ' << list[i].carte.typeCode << ' ' 
                << list[i].carte.plafondHebdo << ' ' << list[i].carte.depenseCouranteHebdo << ' ' << list[i].carte.active << ' '
                << list[i].nbFichiersDepots << ' ' << list[i].nbFichiersRetraits << ' ' << list[i].nbFichiersVirements << '\n';
                
        for (int j = 0; j < list[i].nbTransactions; ++j) {
            fichier << list[i].historique[j].type << '|' << list[i].historique[j].montant << '|' 
                    << list[i].historique[j].devise << '|' << list[i].historique[j].paysDestination << '|' 
                    << list[i].historique[j].nomExpediteur << '|' << list[i].historique[j].suspecte << '\n';
        }
    }
    fichier.close();
}

// Charge l'integralite du registre bancaire stocke dans le dossier banque/
bool chargerDonnees(Client list[], int& nbC) {
    creerDossier("banque");
    std::ofstream creationProvisoire("banque/banque_sauvegarde.txt", std::ios::app); 
    creationProvisoire.close(); 
    
    std::ifstream fichier("banque/banque_sauvegarde.txt"); 
    if (!fichier.is_open()) return false;
    
    if (!(fichier >> nbC) || nbC < 0 || nbC > MAX_CLIENTS) { 
        nbC = 0; 
        return false; 
    }
    
    for (int i = 0; i < nbC; ++i) {
        if (!(fichier >> list[i].identifiant >> list[i].codeSecret >> list[i].sexe >> list[i].pays 
                    >> list[i].soldeCourantFC >> list[i].soldeCourantUSD >> list[i].soldeCourantEUR 
                    >> list[i].soldeCourantCNY >> list[i].soldeCourantGBP
                    >> list[i].soldeEpargneFC >> list[i].soldeEpargneUSD >> list[i].soldeEpargneEUR 
                    >> list[i].soldeEpargneCNY >> list[i].soldeEpargneGBP
                    >> list[i].dettePretUSD >> list[i].joursDepuisPret >> list[i].capitalLevierUSD 
                    >> list[i].prixEntreeLevierRTC >> list[i].taillePositionRTC
                    >> list[i].totalTransfertsEmis >> list[i].messagesNonLus >> list[i].nbTransactions 
                    >> list[i].scoreRisque >> list[i].estGele >> list[i].alerteBlanchiment >> list[i].retroCoins 
                    >> list[i].dernierAccesTimestamp >> list[i].declencheurAchatRTC >> list[i].volumeAchatRTC
                    >> list[i].carte.numero >> list[i].carte.typeCode >> list[i].carte.plafondHebdo 
                    >> list[i].carte.depenseCouranteHebdo >> list[i].carte.active
                    >> list[i].nbFichiersDepots >> list[i].nbFichiersRetraits >> list[i].nbFichiersVirements)) return false;
                    
        fichier.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        for (int j = 0; j < list[i].nbTransactions; ++j) {
            std::getline(fichier, list[i].historique[j].type, '|'); 
            fichier >> list[i].historique[j].montant; 
            fichier.ignore(1, '|');
            std::getline(fichier, list[i].historique[j].devise, '|'); 
            std::getline(fichier, list[i].historique[j].paysDestination, '|'); 
            std::getline(fichier, list[i].historique[j].nomExpediteur, '|'); 
            fichier >> list[i].historique[j].suspecte;
            fichier.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
    fichier.close(); 
    return true;
}

// Injecte les profils internationaux par defaut si la base est vide ou absente
void initialiserClients(Client list[], int& nbC) {
    nbC = 6; 
    long long now = static_cast<long long>(std::time(NULL));
    
    // Client 0 - Jean123 (RDC) -> Profil Franc Congolais
    list[0].identifiant = "Jean123"; list[0].codeSecret = "1234"; list[0].sexe = "M"; list[0].pays = "RDC"; 
    list[0].soldeCourantFC = 1500000; list[0].soldeCourantUSD = 2500; list[0].soldeCourantEUR = 100; list[0].soldeCourantCNY = 0; list[0].soldeCourantGBP = 0;
    list[0].soldeEpargneFC = 3000000; list[0].soldeEpargneUSD = 5000; list[0].soldeEpargneEUR = 0; list[0].soldeEpargneCNY = 0; list[0].soldeEpargneGBP = 0;
    list[0].dettePretUSD = 0; list[0].joursDepuisPret = 0; list[0].capitalLevierUSD = 0; list[0].prixEntreeLevierRTC = 0; list[0].taillePositionRTC = 0;
    list[0].nbTransactions = 0; list[0].totalTransfertsEmis = 0; list[0].messagesNonLus = 0; list[0].scoreRisque = 5; 
    list[0].estGele = false; list[0].alerteBlanchiment = false; list[0].retroCoins = 0; list[0].dernierAccesTimestamp = now; 
    list[0].declencheurAchatRTC = 0; list[0].volumeAchatRTC = 0;
    list[0].nbFichiersDepots = 0; list[0].nbFichiersRetraits = 0; list[0].nbFichiersVirements = 0;
    list[0].carte.numero = "4500123456789012"; list[0].carte.typeCode = "Gold"; list[0].carte.plafondHebdo = 3000; list[0].carte.depenseCouranteHebdo = 0; list[0].carte.active = true;
    creerArborescenceClient(list[0].identifiant);
    ajouterTransaction(list[0], "Initial", 2500, "USD", "RDC", "Systeme", false);

    // Client 1 - Marie456 (France) -> Profil Zone Euro
    list[1].identifiant = "Marie456"; list[1].codeSecret = "5678"; list[1].sexe = "F"; list[1].pays = "France"; 
    list[1].soldeCourantFC = 0; list[1].soldeCourantUSD = 150; list[1].soldeCourantEUR = 1200; list[1].soldeCourantCNY = 0; list[1].soldeCourantGBP = 0;
    list[1].soldeEpargneFC = 0; list[1].soldeEpargneUSD = 0; list[1].soldeEpargneEUR = 4500; list[1].soldeEpargneCNY = 0; list[1].soldeEpargneGBP = 0;
    list[1].dettePretUSD = 0; list[1].joursDepuisPret = 0; list[1].capitalLevierUSD = 0; list[1].prixEntreeLevierRTC = 0; list[1].taillePositionRTC = 0;
    list[1].nbTransactions = 0; list[1].totalTransfertsEmis = 0; list[1].messagesNonLus = 0; list[1].scoreRisque = 10; 
    list[1].estGele = false; list[1].alerteBlanchiment = false; list[1].retroCoins = 0; list[1].dernierAccesTimestamp = now; 
    list[1].declencheurAchatRTC = 0; list[1].volumeAchatRTC = 0;
    list[1].nbFichiersDepots = 0; list[1].nbFichiersRetraits = 0; list[1].nbFichiersVirements = 0;
    list[1].carte.numero = "4900876543210987"; list[1].carte.typeCode = "Platinum"; list[1].carte.plafondHebdo = 7500; list[1].carte.depenseCouranteHebdo = 0; list[1].carte.active = true;
    creerArborescenceClient(list[1].identifiant);
    ajouterTransaction(list[1], "Initial", 1200, "EUR", "France", "Systeme", false);

    // Client 2 - Pierre789 (Canada) -> Profil endetté à risque élevé
    list[2].identifiant = "Pierre789"; list[2].codeSecret = "0000"; list[2].sexe = "M"; list[2].pays = "Canada"; 
    list[2].soldeCourantFC = 0; list[2].soldeCourantUSD = 80; list[2].soldeCourantEUR = 0; list[2].soldeCourantCNY = 0; list[2].soldeCourantGBP = 0;
    list[2].soldeEpargneFC = 0; list[2].soldeEpargneUSD = 0; list[2].soldeEpargneEUR = 0; list[2].soldeEpargneCNY = 0; list[2].soldeEpargneGBP = 0;
    list[2].dettePretUSD = 1200; list[2].joursDepuisPret = 34; list[2].capitalLevierUSD = 0; list[2].prixEntreeLevierRTC = 0; list[2].taillePositionRTC = 0;
    list[2].nbTransactions = 0; list[2].totalTransfertsEmis = 0; list[2].messagesNonLus = 0; list[2].scoreRisque = 55; 
    list[2].estGele = false; list[2].alerteBlanchiment = false; list[2].retroCoins = 0; list[2].dernierAccesTimestamp = now; 
    list[2].declencheurAchatRTC = 0; list[2].volumeAchatRTC = 0;
    list[2].nbFichiersDepots = 0; list[2].nbFichiersRetraits = 0; list[2].nbFichiersVirements = 0;
    list[2].carte.numero = "4100555566667777"; list[2].carte.typeCode = "Classic"; list[2].carte.plafondHebdo = 1000; list[2].carte.depenseCouranteHebdo = 0; list[2].carte.active = true;
    creerArborescenceClient(list[2].identifiant);
    ajouterTransaction(list[2], "Initial", 80, "USD", "Canada", "Systeme", false);

    // Client 3 - Kader243 (RDC) -> Profil Spéculateur Crypto RTC
    list[3].identifiant = "Kader243"; list[3].codeSecret = "3333"; list[3].sexe = "M"; list[3].pays = "RDC"; 
    list[3].soldeCourantFC = 450000; list[3].soldeCourantUSD = 10; list[3].soldeCourantEUR = 0; list[3].soldeCourantCNY = 400; list[3].soldeCourantGBP = 0;
    list[3].soldeEpargneFC = 1200000; list[3].soldeEpargneUSD = 0; list[3].soldeEpargneEUR = 0; list[3].soldeEpargneCNY = 0; list[3].soldeEpargneGBP = 0;
    list[3].dettePretUSD = 0; list[3].joursDepuisPret = 0; list[3].capitalLevierUSD = 0; list[3].prixEntreeLevierRTC = 0; list[3].taillePositionRTC = 0;
    list[3].nbTransactions = 0; list[3].totalTransfertsEmis = 0; list[3].messagesNonLus = 0; list[3].scoreRisque = 15; 
    list[3].estGele = false; list[3].alerteBlanchiment = false; list[3].retroCoins = 14; list[3].dernierAccesTimestamp = now; 
    list[3].declencheurAchatRTC = 0; list[3].volumeAchatRTC = 0;
    list[3].nbFichiersDepots = 0; list[3].nbFichiersRetraits = 0; list[3].nbFichiersVirements = 0;
    list[3].carte.numero = "4200999988887777"; list[3].carte.typeCode = "Gold"; list[3].carte.plafondHebdo = 3000; list[3].carte.depenseCouranteHebdo = 0; list[3].carte.active = true;
    creerArborescenceClient(list[3].identifiant);
    ajouterTransaction(list[3], "Initial", 450000, "FC", "RDC", "Systeme", false);

    // Client 4 - Wong888 (Chine) -> Profil Grand Compte en Yuan
    list[4].identifiant = "Wong888"; list[4].codeSecret = "8888"; list[4].sexe = "M"; list[4].pays = "Chine"; 
    list[4].soldeCourantFC = 0; list[4].soldeCourantUSD = 500; list[4].soldeCourantEUR = 0; list[4].soldeCourantCNY = 25000; list[4].soldeCourantGBP = 0;
    list[4].soldeEpargneFC = 0; list[4].soldeEpargneUSD = 0; list[4].soldeEpargneEUR = 0; list[4].soldeEpargneCNY = 60000; list[4].soldeEpargneGBP = 0;
    list[4].dettePretUSD = 0; list[4].joursDepuisPret = 0; list[4].capitalLevierUSD = 0; list[4].prixEntreeLevierRTC = 0; list[4].taillePositionRTC = 0;
    list[4].nbTransactions = 0; list[4].totalTransfertsEmis = 0; list[4].messagesNonLus = 0; list[4].scoreRisque = 8; 
    list[4].estGele = false; list[4].alerteBlanchiment = false; list[4].retroCoins = 0; list[4].dernierAccesTimestamp = now; 
    list[4].declencheurAchatRTC = 0; list[4].volumeAchatRTC = 0;
    list[4].nbFichiersDepots = 0; list[4].nbFichiersRetraits = 0; list[4].nbFichiersVirements = 0;
    list[4].carte.numero = "4600111122223333"; list[4].carte.typeCode = "Platinum"; list[4].carte.plafondHebdo = 7500; list[4].carte.depenseCouranteHebdo = 0; list[4].carte.active = true;
    creerArborescenceClient(list[4].identifiant);
    ajouterTransaction(list[4], "Initial", 25000, "CNY", "Chine", "Systeme", false);

    // Client 5 - Smith777 (Royaume-Uni) -> Profil Premium en Livre Sterling
    list[5].identifiant = "Smith777"; list[5].codeSecret = "7777"; list[5].sexe = "M"; list[5].pays = "Royaume-Uni"; 
    list[5].soldeCourantFC = 0; list[5].soldeCourantUSD = 100; list[5].soldeCourantEUR = 0; list[5].soldeCourantCNY = 0; list[5].soldeCourantGBP = 3500;
    list[5].soldeEpargneFC = 0; list[5].soldeEpargneUSD = 0; list[5].soldeEpargneEUR = 0; list[5].soldeEpargneCNY = 0; list[5].soldeEpargneGBP = 9000;
    list[5].dettePretUSD = 0; list[5].joursDepuisPret = 0; list[5].capitalLevierUSD = 0; list[5].prixEntreeLevierRTC = 0; list[5].taillePositionRTC = 0;
    list[5].nbTransactions = 0; list[5].totalTransfertsEmis = 0; list[5].messagesNonLus = 0; list[5].scoreRisque = 12; 
    list[5].estGele = false; list[5].alerteBlanchiment = false; list[5].retroCoins = 2; list[5].dernierAccesTimestamp = now; 
    list[5].declencheurAchatRTC = 0; list[5].volumeAchatRTC = 0;
    list[5].nbFichiersDepots = 0; list[5].nbFichiersRetraits = 0; list[5].nbFichiersVirements = 0;
    list[5].carte.numero = "4700444455556666"; list[5].carte.typeCode = "Gold"; list[5].carte.plafondHebdo = 3000; list[5].carte.depenseCouranteHebdo = 0; list[5].carte.active = true;
    creerArborescenceClient(list[5].identifiant);
    ajouterTransaction(list[5], "Initial", 3500, "GBP", "Royaume-Uni", "Systeme", false);
}

// Oriente dynamiquement vers le compartiment de solde courant (1:FC, 2:USD, 3:EUR, 4:CNY, 5:GBP)
double* soldeCourant(Client& c, int dev) { 
    if (dev == 1) return &c.soldeCourantFC; if (dev == 2) return &c.soldeCourantUSD; if (dev == 3) return &c.soldeCourantEUR; 
    if (dev == 4) return &c.soldeCourantCNY; if (dev == 5) return &c.soldeCourantGBP; return NULL; 
}

// Oriente dynamiquement vers le compartiment de solde epargne
double* soldeEpargne(Client& c, int dev) { 
    if (dev == 1) return &c.soldeEpargneFC; if (dev == 2) return &c.soldeEpargneUSD; if (dev == 3) return &c.soldeEpargneEUR; 
    if (dev == 4) return &c.soldeEpargneCNY; if (dev == 5) return &c.soldeEpargneGBP; return NULL; 
}

// Renvoie le symbole standard de la devise demandee
std::string nomDevise(int dev) { 
    if (dev == 1) return "FC"; if (dev == 2) return "USD"; if (dev == 3) return "EUR"; if (dev == 4) return "CNY"; if (dev == 5) return "GBP"; return ""; 
}

// Convertisseur Forex utilisant le Dollar US comme monnaie pivot de reference
double convertir(double m, int de, int vers, double tUSD_FC, double tEUR_USD, double tUSD_CNY, double tGBP_USD) {
    if (de == vers) return m; double mUSD = m; 
    if (de == 1) mUSD = m / tUSD_FC; else if (de == 3) mUSD = m * tEUR_USD; else if (de == 4) mUSD = m / tUSD_CNY; else if (de == 5) mUSD = m * tGBP_USD;
    if (vers == 1) return mUSD * tUSD_FC; else if (vers == 2) return mUSD; else if (vers == 3) return mUSD / tEUR_USD; else if (vers == 4) return mUSD * tUSD_CNY; else if (vers == 5) return mUSD / tGBP_USD; return -1.0;
}

// Génère un histogramme en texte représentant les encours de dettes cumulés par pays
void afficherHistogrammeDettes(const Client list[], int nbC) {
    ecranBleu(); std::cout << " === [ANALYSE] HISTOGRAMME GLOBAL DES DETTES ===\n\n";
    double dRDC = 0, dFR = 0, dCA = 0, dCH = 0, dUK = 0;
    for (int i = 0; i < nbC; ++i) { 
        if (list[i].pays == "RDC") dRDC += list[i].dettePretUSD; 
        else if (list[i].pays == "France") dFR += list[i].dettePretUSD; 
        else if (list[i].pays == "Canada") dCA += list[i].dettePretUSD; 
        else if (list[i].pays == "Chine") dCH += list[i].dettePretUSD;
        else if (list[i].pays == "Royaume-Uni") dUK += list[i].dettePretUSD;
    }
    const double ESC = 100.0; 
    std::string p[] = {"RDC", "France", "Canada", "Chine", "Royaume-Uni"}; 
    double d[] = {dRDC, dFR, dCA, dCH, dUK};
    for (int i = 0; i < 5; ++i) {
        std::cout << " " << p[i]; for (size_t k = p[i].length(); k < 12; ++k) std::cout << " "; std::cout << "["; int stars = static_cast<int>(d[i] / ESC); if (stars > 20) stars = 20;
        for (int j = 0; j < 20; ++j) { if (j < stars) std::cout << "*"; else std::cout << " "; }
        std::cout << "] " << d[i] << " USD imputes\n";
    }
}

// Génère un graphique structurel en art ASCII représentant les parts de marché des monnaies sous gestion
void afficherCamembertDevises(const Client list[], int nbC, double tUSD_FC, double tEUR_USD, double tUSD_CNY, double tGBP_USD) {
    ecranBleu(); std::cout << " === [MACRO] GRAPH DE REPARTITION MONETAIRE ===\n\n";
    double tFC = 0, tUSD = 0, tEUR = 0, tCNY = 0, tGBP = 0; 
    for (int i = 0; i < nbC; ++i) { 
        tFC += list[i].soldeCourantFC + list[i].soldeEpargneFC; 
        tUSD += list[i].soldeCourantUSD + list[i].soldeEpargneUSD; 
        tEUR += list[i].soldeCourantEUR + list[i].soldeEpargneEUR; 
        tCNY += list[i].soldeCourantCNY + list[i].soldeEpargneCNY; 
        tGBP += list[i].soldeCourantGBP + list[i].soldeEpargneGBP; 
    }
    double mTot = (tFC / tUSD_FC) + tUSD + (tEUR * tEUR_USD) + (tCNY / tUSD_CNY) + (tGBP * tGBP_USD); 
    if (mTot <= 0) return;
    
    double pFC = ((tFC / tUSD_FC) / mTot) * 100.0, pUSD = (tUSD / mTot) * 100.0, pEUR = ((tEUR * tEUR_USD) / mTot) * 100.0;
    double pCNY = ((tCNY / tUSD_CNY) / mTot) * 100.0, pGBP = ((tGBP * tGBP_USD) / mTot) * 100.0;
    
    std::cout << "       ______       \n     /        \\     \n    /   FC     \\    Part de marche mondiale :\n   |   " << (pFC > 20 ? "@@@@" : "....") << "    |   -> FC  : " << pFC << " %\n   |   USD      |   -> USD : " << pUSD << " %\n    \\   EUR    /    -> EUR : " << pEUR << " %\n     \\________/     -> CNY : " << pCNY << " %\n                    -> GBP : " << pGBP << " %\n\n Masse complete sous gestion interbancaire : " << mTot << " USD\n";
}

// Dessine une courbe boursière dynamique défilante point par point pour matérialiser le marché direct
void afficherMarcheLiveForex(const double historique[], int taille, std::string labelDevise) {
    ecranBleu(); std::cout << " === [STREAM FOREX] GRAPHIQUE TICKER TEMPS REEL : " << labelDevise << " ===\n\n";
    double mx = historique[0], mn = historique[0];
    for (int i = 1; i < taille; ++i) { if (historique[i] > mx) mx = historique[i]; if (historique[i] < mn) mn = historique[i]; }
    if (mx == mn) mx += 2.0;
    for (int y = 5; y >= 0; --y) {
        double th = mn + (y * (mx - mn) / 5); printf(" %9.2f |", th);
        for (int x = 0; x < taille; ++x) {
            int h = static_cast<int>((historique[x] - mn) / (mx - mn) * 5);
            if (h == y) { if (x > 0 && historique[x] > historique[x-1]) std::cout << "/"; else if (x > 0 && historique[x] < historique[x-1]) std::cout << "\\"; else std::cout << "_"; }
            else std::cout << " ";
        }
        std::cout << "\n";
    }
    std::cout << "           -------------------------------------\n           [Flux Historique] -------------> [Tick Direct]\n";
}

// Dessine une courbe retraçant l'évolution du solde du client lors de ses dernières opérations
void afficherCourbeSolde(const Client& c) {
    ecranBleu(); std::cout << " === COURBE D'EVOLUTION GENERALE DE TRESORERIE (USD) ===\n\n";
    if (c.nbTransactions == 0) return; double pts[10]; for(int i = 0; i < 10; ++i) pts[i] = 0.0;
    int nP = (c.nbTransactions > 10) ? 10 : c.nbTransactions; int st = c.nbTransactions - nP; double sSim = c.soldeCourantUSD;
    for (int i = nP - 1; i >= 0; --i) { pts[i] = sSim; const Transaction& t = c.historique[st + i]; if (t.type == "Depot" && t.devise == "USD") sSim -= t.montant; if (t.type == "Retrait" && t.devise == "USD") sSim += t.montant; }
    double mx = pts[0], mn = pts[0]; for (int i = 1; i < nP; ++i) { if (pts[i] > mx) mx = pts[i]; if (pts[i] < mn) mn = pts[i]; } if (mx == mn) mx += 10.0;
    for (int y = 5; y >= 0; --y) {
        double th = mn + (y * (mx - mn) / 5); printf(" %7.1f$ |", th);
        for (int x = 0; x < nP; ++x) { int h = static_cast<int>((pts[x] - mn) / (mx - mn) * 5); if (h == y) std::cout << ( (x>0 && pts[x]>pts[x-1]) ? " / " : ((x>0 && pts[x]<pts[x-1]) ? " \\ " : " _ ") ); else std::cout << "   "; }
        std::cout << "\n";
    }
}

// Affiche un diagramme ASCII comparatif entre les avoirs réels et les passifs (crédit) du client
void afficherGraphiqueSanteClient(const Client& c) {
    ecranBleu(); std::cout << " === [PORTAIL CREDIT] DIAGRAMME FORTUNE VS ENDETTEMENT ===\n\n";
    double fTot = c.soldeCourantUSD + c.soldeEpargneUSD, dUSD = c.dettePretUSD;
    int bF = static_cast<int>(fTot / 300.0), bD = static_cast<int>(dUSD / 300.0); if(bF > 20) bF = 20; if(bD > 20) bD = 20;
    std::cout << " Fortune Totale ["; for(int i=0; i<20; ++i) std::cout << (i < bF ? "#" : " "); std::cout << "] " << fTot << " USD\n";
    std::cout << " Dette Carence  ["; for(int i=0; i<20; ++i) std::cout << (i < bD ? "X" : " "); std::cout << "] " << dUSD << " USD\n";
}

// Exporte un fichier .txt global contenant le statut financier de référence de l'utilisateur
void exporterRecu(const Client& c) {
    std::string nm = "clients/" + c.identifiant + "/Facture_" + c.identifiant + ".txt"; std::ofstream f(nm.c_str()); if (!f.is_open()) return;
    f << "========================================\n" << "          BANK-GLODY-CLAVER TICKET      \n" << "========================================\n" << " Client ID : " << c.identifiant << "\n Solde : " << c.soldeCourantUSD << " USD\n Dette : " << c.dettePretUSD << " USD\n"; f.close();
    std::cout << " [SUCCES] Releve bancaire genere sur le disque.\n"; ecrireAudit(std::string("Exportation ticket : ") + c.identifiant);
}

int main() {
    // Déclenchement automatique de l'écran de chargement immersif au démarrage
    effetChargementInitial();

    std::srand(static_cast<unsigned int>(std::time(NULL))); 
    int nbC = 0; bool maint = false; Client list[MAX_CLIENTS];
    
    // Taux de change initiaux modifiables
    double tauxUSD_FC = 2800.0, tauxEUR_USD = 1.09, coursRetroCoin = 25.5, tauxUSD_CNY = 7.15, tauxGBP_USD = 1.28; 
    double streamFC[FILET_FOREX_SIZE], streamEUR[FILET_FOREX_SIZE], streamRTC[FILET_FOREX_SIZE];
    for(int i=0; i<FILET_FOREX_SIZE; ++i) { streamFC[i] = 2800.0; streamEUR[i] = 1.09; streamRTC[i] = 25.5; }
    
    if (!chargerDonnees(list, nbC)) { initialiserClients(list, nbC); sauvegarderDonnees(list, nbC); }
    int opt = 0;
    
    while (opt != 3) {
        // --- MOTEUR DE CRISES ÉCONOMIQUES GÉOPOLITIQUES EN TEMPS RÉEL ---
        int ev = std::rand() % 100;
        if (ev < 5) { ecranJaune(); std::cout << "\n [ALERTE MARCHE] Gisement de cuivre decouvert en RDC ! Le FC bondit.\n"; tauxUSD_FC *= 0.85; attendreRetour(); } 
        else if (ev >= 5 && ev < 10) { ecranRouge(); std::cout << "\n [ALERTE MARCHE] Crise des puces a Taiwan ! Le Yuan (CNY) chute.\n"; tauxUSD_CNY *= 1.12; attendreRetour(); } 
        else if (ev >= 10 && ev < 15) { ecranVert(); std::cout << "\n [ALERTE MARCHE] Elon Musk tweete ! La crypto explose de +50%.\n"; coursRetroCoin *= 1.50; attendreRetour(); } 
        else if (ev >= 15 && ev < 18) { ecranRouge(); std::cout << "\n [ALERTE MARCHE] Attaque cybernetique ! Le RetroCoin chute de -40%.\n"; coursRetroCoin *= 0.60; if(coursRetroCoin < 0.5) coursRetroCoin = 0.5; attendreRetour(); }

        tauxUSD_FC += sin(static_cast<double>(time(NULL)) * 0.04) * 4.0 + ((rand() % 101) - 50) * 0.1;
        tauxEUR_USD += cos(static_cast<double>(time(NULL)) * 0.02) * 0.002 + ((rand() % 41) - 20) * 0.0005;
        coursRetroCoin += ((rand() % 61) - 30) * 0.15; if(coursRetroCoin < 0.5) coursRetroCoin = 0.5;
        for(int i=1; i<FILET_FOREX_SIZE; ++i) { streamFC[i-1] = streamFC[i]; streamEUR[i-1] = streamEUR[i]; streamRTC[i-1] = streamRTC[i]; }
        streamFC[FILET_FOREX_SIZE-1] = tauxUSD_FC; streamEUR[FILET_FOREX_SIZE-1] = tauxEUR_USD; streamRTC[FILET_FOREX_SIZE-1] = coursRetroCoin;

        // Évaluation des risques de marge et exécution des automates
        for(int i = 0; i < nbC; ++i) {
            if(list[i].taillePositionRTC > 0) {
                double pnl = list[i].taillePositionRTC * (coursRetroCoin - list[i].prixEntreeLevierRTC);
                if((list[i].capitalLevierUSD + pnl) <= 0) { ecranRouge(); std::cout << "\n [!!! MARGIN CALL !!!] Liquidation position Levier pour " << list[i].identifiant << " !\n"; list[i].capitalLevierUSD = 0; list[i].taillePositionRTC = 0; list[i].prixEntreeLevierRTC = 0; list[i].scoreRisque = 100; list[i].estGele = true; sauvegarderDonnees(list, nbC); attendreRetour(); }
            }
            if(list[i].declencheurAchatRTC > 0 && coursRetroCoin <= list[i].declencheurAchatRTC) {
                double ct = list[i].volumeAchatRTC * coursRetroCoin;
                if(ct <= list[i].soldeCourantUSD && list[i].volumeAchatRTC > 0) { list[i].soldeCourantUSD -= ct; list[i].retroCoins += list[i].volumeAchatRTC; ajouterTransaction(list[i], "AUTO BUY RTC", list[i].volumeAchatRTC, "RTC", list[i].pays, "Automate", false); std::ostringstream ss; ss << list[i].volumeAchatRTC; ecrireAudit(std::string("[AUTOMATE] Execute pour ") + list[i].identifiant + " : " + ss.str() + " RTC"); list[i].declencheurAchatRTC = 0; list[i].volumeAchatRTC = 0; }
            }
        }

        menuAccueil(); if (!(std::cin >> opt)) { viderEntree(); continue; }
        if (opt == 3) break;
        
        // --- ESPACE DE CONNEXION ADMINISTRATEUR ---
        if (opt == 2) {
            ecranBleu(); std::string aCode; std::cout << " === CONNEXION BACK-OFFICE ADMINISTRATEUR ===\n Cle Securite (*) : "; aCode = lireCodeEtoile();
            if (aCode != "admin123") { std::cout << " [ACCES REFUSE]\n"; attendreRetour(); continue; }
            int chA = 0;
            while (chA != 12) {
                menuAdmin(); if (!(std::cin >> chA)) { viderEntree(); continue; }
                if (chA == 1) {
                    if (nbC >= MAX_CLIENTS) { attendreRetour(); continue; }
                    ecranBleu(); Client n; std::cout << " ID unique du client : "; std::cin >> n.identifiant; std::cout << " Code secret : "; std::cin >> n.codeSecret; std::cout << " Sexe (M/F) : "; std::cin >> n.sexe; std::cout << " Pays : "; std::cin >> n.pays; std::cout << " Solde initial (USD) : ";
                    if (!(std::cin >> n.soldeCourantUSD) || n.soldeCourantUSD < 0) { viderEntree(); continue; }
                    n.soldeCourantFC = 0; n.soldeCourantEUR = 0; n.soldeCourantCNY = 0; n.soldeCourantGBP = 0; n.soldeEpargneFC = 0; n.soldeEpargneUSD = 0; n.soldeEpargneEUR = 0; n.soldeEpargneCNY = 0; n.soldeEpargneGBP = 0; n.dettePretUSD = 0; n.joursDepuisPret = 0; n.capitalLevierUSD = 0; n.prixEntreeLevierRTC = 0; n.taillePositionRTC = 0; n.totalTransfertsEmis = 0; n.messagesNonLus = 0; n.nbTransactions = 0; n.scoreRisque = 0; n.estGele = false; n.alerteBlanchiment = false; n.retroCoins = 0; n.declencheurAchatRTC = 0; n.volumeAchatRTC = 0; n.dernierAccesTimestamp = static_cast<long long>(std::time(NULL));
                    n.nbFichiersDepots = 0; n.nbFichiersRetraits = 0; n.nbFichiersVirements = 0;
                    n.carte.numero = "4000" + n.codeSecret + "11112222"; n.carte.typeCode = "Classic"; n.carte.plafondHebdo = 1500; n.carte.depenseCouranteHebdo = 0; n.carte.active = true;
                    creerArborescenceClient(n.identifiant);
                    list[nbC++] = n; sauvegarderDonnees(list, nbC); attendreRetour();
                }
                else if (chA == 2) { ecranBleu(); Client cp[MAX_CLIENTS]; for (int i = 0; i < nbC; ++i) cp[i] = list[i]; for (int i = 0; i < nbC - 1; ++i) for (int j = 0; j < nbC - i - 1; ++j) if (cp[j].soldeCourantUSD < cp[j + 1].soldeCourantUSD) { Client t = cp[j]; cp[j] = cp[j + 1]; cp[j + 1] = t; } for (int i = 0; i < nbC; ++i) std::cout << " - " << cp[i].identifiant << " : " << cp[i].soldeCourantUSD << " USD\n"; attendreRetour(); }
                else if (chA == 3) { int subM = 0; std::cout << " 1.USD/FC | 2.EUR/USD | 3.RetroCoin | 0.Annuler : "; std::cin >> subM; if(subM==1) afficherMarcheLiveForex(streamFC, FILET_FOREX_SIZE, "USD / FC"); if(subM==2) afficherMarcheLiveForex(streamEUR, FILET_FOREX_SIZE, "EUR / USD"); if(subM==3) afficherMarcheLiveForex(streamRTC, FILET_FOREX_SIZE, "RETROCOIN / USD"); attendreRetour(); }
                else if (chA == 4) { ecranBleu(); const std::string pays[] = {"RDC", "France", "Canada", "Chine", "Royaume-Uni"}; for (int pi = 0; pi < 5; ++pi) { double tot = 0.0; for (int x = 0; x < nbC; ++x) for (int y = 0; y < list[x].nbTransactions; ++y) if (list[x].historique[y].paysDestination == pays[pi] && list[x].historique[y].type.find("Envoi") != std::string::npos) tot += list[x].historique[y].montant; std::cout << "  -> Vers [" << pays[pi] << "] : " << tot << " USD\n"; } attendreRetour(); }
                else if (chA == 5) { ecranJaune(); std::cout << " === [SURVEILLANCE COMPTEUR FICHIERS & TRACFIN] ===\n\n"; for (int i = 0; i < nbC; ++i) { std::cout << " Client: " << list[i].identifiant << " | Doc Depots: " << list[i].nbFichiersDepots << " | Doc Retraits: " << list[i].nbFichiersRetraits << " | Risque: " << list[i].scoreRisque << "% " << (list[i].alerteBlanchiment ? "[!!! ALERTE TRACFIN !!!]" : "") << "\n"; } attendreRetour(); }
                else if (chA == 6) { maint = !maint; std::cout << " Maintenance commutee : " << (maint ? "ACTIVE" : "DESACTIVE") << "\n"; attendreRetour(); }
                else if (chA == 7) { afficherHistogrammeDettes(list, nbC); attendreRetour(); }
                else if (chA == 8) { afficherCamembertDevises(list, nbC, tauxUSD_FC, tauxEUR_USD, tauxUSD_CNY, tauxGBP_USD); attendreRetour(); }
                else if (chA == 9) { std::string rId; std::cout << " Lever le gel de quel ID client : "; std::cin >> rId; for(int i=0; i<nbC; ++i) { if(list[i].identifiant == rId) { list[i].estGele = false; list[i].alerteBlanchiment = false; list[i].scoreRisque = 10; sauvegarderDonnees(list, nbC); std::cout << " Compte revalide.\n"; break; } } attendreRetour(); }
                else if (chA == 10) { for(int i=0; i<nbC; ++i) { list[i].soldeCourantUSD *= 0.98; list[i].soldeCourantFC *= 0.98; list[i].soldeCourantEUR *= 0.98; } sauvegarderDonnees(list, nbC); std::cout << " Taxe 2% executee.\n"; attendreRetour(); }
                else if (chA == 11) { ecranBleu(); std::cout << "=== AJUSTEMENT DIRECT DES COURS ===\n"; std::cout << "USD/FC : "; std::cin >> tauxUSD_FC; std::cout << "EUR/USD : "; std::cin >> tauxEUR_USD; std::cout << "USD/CNY : "; std::cin >> tauxUSD_CNY; std::cout << "GBP/USD : "; std::cin >> tauxGBP_USD; attendreRetour(); }
            }
            effetMatrix(); continue;
        }
        
        // --- ESPACE DE NAVIGATION PORTAIL CLIENT ---
        if (opt == 1) {
            std::string id, code; int idx = -1; bool ok = false; int tent = 0;
            while (tent < 3 && !ok) {
                ecranBleu(); std::cout << " === PORTAIL CLIENT SECURISE ===\n ID : "; std::cin >> id; std::cout << " Code (*) : "; viderEntree(); code = lireCodeEtoile();
                for (int i = 0; i < nbC; ++i) { if (list[i].identifiant == id && list[i].codeSecret == code) { idx = i; ok = true; break; } }
                if (!ok) { ++tent; std::cout << " Incorrect. " << (3 - tent) << " restants.\n"; attendreRetour(); }
            }
            if (!ok) continue; if (maint) { ecranRouge(); std::cout << " [MAINTENANCE EN COURS]\n"; attendreRetour(); continue; } if (list[idx].estGele) { ecranRouge(); std::cout << " [ACCES INTERDIT : COMPTE GELE]\n"; attendreRetour(); continue; }

            long long now = static_cast<long long>(std::time(NULL)); long long delta = now - list[idx].dernierAccesTimestamp;
            if (delta >= 10) { double intEp = list[idx].soldeEpargneUSD * 0.02; if (intEp > 0) { list[idx].soldeEpargneUSD += intEp; ajouterTransaction(list[idx], "Interet Epargne", intEp, "USD", list[idx].pays, "Banque", false); } list[idx].soldeCourantUSD *= 0.995; } 
            list[idx].dernierAccesTimestamp = now;

            if (list[idx].dettePretUSD > 0) { list[idx].joursDepuisPret += 1; if(list[idx].joursDepuisPret > 45) { ecranRouge(); std::cout << " [SAISIE AUTOMATIQUE] Plus de 45 jours de retard ! Prelevement de 10%.\n"; double pr = list[idx].dettePretUSD * 0.1; if(list[idx].soldeCourantUSD >= pr) { list[idx].soldeCourantUSD -= pr; list[idx].dettePretUSD -= pr; } else { list[idx].estGele = true; } sauvegarderDonnees(list, nbC); attendreRetour(); if(list[idx].estGele) continue; } }

            int chC = 0;
            while (chC != 16) {
                menuClient(&list[idx], coursRetroCoin); if (!(std::cin >> chC)) { viderEntree(); continue; }
                
            if (chC == 1) { ecranBleu(); std::cout << "=== VOS DISPONIBILITES COURANTES ===\n - FC (Franc Congolais)  : " << list[idx].soldeCourantFC << "\n - USD (Dollar Americain): " << list[idx].soldeCourantUSD << "\n - EUR (Euro Europeen)   : " << list[idx].soldeCourantEUR << "\n - CNY (Yuan Chinois)    : " << list[idx].soldeCourantCNY << "\n - GBP (Livre Sterling)  : " << list[idx].soldeCourantGBP << "\n\n=== COMPTES PLACEMENT EPARGNE ===\n - FC  : " << list[idx].soldeEpargneFC << "\n - USD : " << list[idx].soldeEpargneUSD << "\n - EUR : " << list[idx].soldeEpargneEUR << "\n\n Score Risque Surveillance : " << list[idx].scoreRisque << " % | Position de Marge Levier : " << list[idx].capitalLevierUSD << " USD\n"; attendreRetour(); }
                else if (chC == 2) { 
                    int ty, dev; double mt; std::cout << " Compartiment (1.Courant | 2.Epargne | 0.Annuler) : "; std::cin >> ty; if(ty==0) continue;
                    std::cout << " Devise (1.FC | 2.USD | 3.EUR | 4.CNY | 5.GBP | 0.Annuler) : "; std::cin >> dev; if(dev==0) continue;
                    std::cout << " Montant physique a deposer : "; std::cin >> mt; 
                    double* s = (ty == 1) ? soldeCourant(list[idx], dev) : soldeEpargne(list[idx], dev); 
                    if (s && mt > 0) { 
                        if(list[idx].nbFichiersDepots >= 50) { std::cout << " [BLOCAGE DISQUE] Trop de reçus generes pour votre session. Contacter l'administrateur.\n"; attendreRetour(); continue; }
                        *s += mt; if(mt > 10000) { list[idx].alerteBlanchiment = true; list[idx].scoreRisque += 30; std::cout << " [ALERTE ROUGE TRACFIN ACTIVEE] Flux de capitaux suspect transmis au gouvernement.\n"; } 
                        list[idx].nbFichiersDepots++; std::string nF = "clients/" + list[idx].identifiant + "/depots/depot_" + nomDevise(dev) + ".txt"; std::ofstream fD(nF.c_str(), std::ios::app); fD << "Depot: " << mt << " " << nomDevise(dev) << " effectue.\n"; fD.close();
                        ajouterTransaction(list[idx], "Depot", mt, nomDevise(dev), list[idx].pays, "Moi", mt > 10000); sauvegarderDonnees(list, nbC); 
                    } 
                    attendreRetour(); 
                }
                else if (chC == 3) { int dev; double mt; std::cout << " Devise du retrait (1.FC-5.GBP | 0.Annuler) : "; std::cin >> dev; if(dev==0) continue; std::cout << " Montant : "; std::cin >> mt; double* s = soldeCourant(list[idx], dev); if(s && mt > 0 && *s >= mt) { if(list[idx].carte.active && (list[idx].carte.depenseCouranteHebdo + mt) > list[idx].carte.plafondHebdo) { std::cout << " [OPERATION REFUSEE] Le plafond hebdomadaire de votre carte refuse le debit.\n"; } else { *s -= mt; if(list[idx].carte.active) list[idx].carte.depenseCouranteHebdo += mt; list[idx].nbFichiersRetraits++; std::string nF = "clients/" + list[idx].identifiant + "/retraits/retrait_" + nomDevise(dev) + ".txt"; std::ofstream fR(nF.c_str(), std::ios::app); fR << "Retrait: " << mt << " " << nomDevise(dev) << " valide.\n"; fR.close(); ajouterTransaction(list[idx], "Retrait", mt, nomDevise(dev), list[idx].pays, "Moi", false); sauvegarderDonnees(list, nbC); } } else { std::cout << " Provision insuffisante.\n"; } attendreRetour(); }
                else if (chC == 4) { int sens, dev; double mt; std::cout << " Direction (1.Courant->Epargne | 2.Epargne->Courant | 0.Annuler) : "; std::cin >> sens; if(sens==0) continue; std::cout << " Devise : "; std::cin >> dev; std::cout << " Volume de transfert : "; std::cin >> mt; double* src = (sens == 1) ? soldeCourant(list[idx], dev) : soldeEpargne(list[idx], dev); double* dst = (sens == 1) ? soldeEpargne(list[idx], dev) : soldeCourant(list[idx], dev); if(src && dst && mt > 0 && *src >= mt) { *src -= mt; *dst += mt; sauvegarderDonnees(list, nbC); std::cout << " Transfert interne execute.\n"; } attendreRetour(); }
                else if (chC == 5) { int de, vers; double mt; std::cout << " Devise a vendre (1-5) : "; std::cin >> de; std::cout << " Devise a acheter (1-5 | 0.Annuler) : "; std::cin >> vers; if(vers==0) continue; std::cout << " Montant de la vente : "; std::cin >> mt; double* s_de = soldeCourant(list[idx], de); double* s_vers = soldeCourant(list[idx], vers); if(s_de && s_vers && mt > 0 && *s_de >= mt) { double res = convertir(mt, de, vers, tauxUSD_FC, tauxEUR_USD, tauxUSD_CNY, tauxGBP_USD); if(res >= 0) { *s_de -= mt; *s_vers += res; sauvegarderDonnees(list, nbC); std::cout << " Troc Forex effectue au cours actuel.\n"; } } attendreRetour(); }
                else if (chC == 6) { std::string destId, destP; double mt; std::cout << " ID Unique du destinataire : "; std::cin >> destId; std::cout << " Pays de destination : "; std::cin >> destP; int iDst = -1; for(int i=0; i<nbC; ++i) if(list[i].identifiant == destId) { iDst = i; break; } if(iDst != -1) { std::cout << " Montant a envoyer (USD | 0.Annuler) : "; std::cin >> mt; if(mt == 0) continue; if(mt > 0 && mt <= list[idx].soldeCourantUSD) { effetMatrix(); list[idx].soldeCourantUSD -= mt; list[iDst].soldeCourantUSD += mt; list[idx].totalTransfertsEmis++; if(mt > 8000) { list[idx].alerteBlanchiment = true; list[idx].scoreRisque += 40; } list[idx].nbFichiersVirements++; std::string nF = "clients/" + list[idx].identifiant + "/virements/virement_emis.txt"; std::ofstream fV(nF.c_str(), std::ios::app); fV << "Virement international de " << mt << " USD vers " << destId << "\n"; fV.close(); ajouterTransaction(list[idx], "Envoi -> " + destId, mt, "USD", destP, list[idx].identifiant, mt > 8000); ajouterTransaction(list[iDst], "Recu de " + list[idx].identifiant, mt, "USD", destP, list[idx].identifiant, mt > 8000); list[iDst].messagesNonLus++; sauvegarderDonnees(list, nbC); } } else { std::cout << " Destinataire introuvable.\n"; } attendreRetour(); }
                else if (chC == 7) { int mP; std::cout << " 1.Pret (Max 5000 USD) | 2.Rembourser | 0.Annuler : "; std::cin >> mP; if(mP == 1 && list[idx].dettePretUSD == 0 && list[idx].scoreRisque < 50) { double mt; std::cout << " Demande d'emprunt (USD) : "; std::cin >> mt; if(mt > 0 && mt <= 5000) { list[idx].soldeCourantUSD += mt; list[idx].dettePretUSD = mt; list[idx].joursDepuisPret = 0; ajouterTransaction(list[idx], "Emprunt", mt, "USD", list[idx].pays, "Banque", false); sauvegarderDonnees(list, nbC); } } else if(mP == 2 && list[idx].dettePretUSD > 0) { double mt; std::cout << " Remboursement volontaire (USD) : "; std::cin >> mt; if(mt > 0 && list[idx].soldeCourantUSD >= mt) { if(mt > list[idx].dettePretUSD) mt = list[idx].dettePretUSD; list[idx].soldeCourantUSD -= mt; list[idx].dettePretUSD -= mt; sauvegarderDonnees(list, nbC); } } attendreRetour(); }
                else if (chC == 8) { ecranBleu(); std::cout << " === CHAT INTERNE COMMUNICATIONS BANQUE ===\n\n"; if(list[idx].messagesNonLus > 0) std::cout << " [SYSTEME] Alerte de securite : Vous avez recu des virements entrants. Verifiez vos soldes.\n"; else std::cout << " Aucun message non lu dans votre boite.\n"; list[idx].messagesNonLus = 0; attendreRetour(); }
                else if (chC == 9) { afficherCourbeSolde(list[idx]); attendreRetour(); }
                else if (chC == 10) { ecranBleu(); int sM; std::cout << " Cours RTC : " << coursRetroCoin << " USD\n 1. Achat Spot | 2. Vente Spot | 0.Annuler : "; std::cin >> sM; if(sM == 1) { double q; std::cout << " Quantite de RTC souhaitee : "; std::cin >> q; double ct = q * coursRetroCoin; if(list[idx].soldeCourantUSD >= ct && q > 0) { list[idx].soldeCourantUSD -= ct; list[idx].retroCoins += q; ajouterTransaction(list[idx], "BUY RTC", q, "RTC", list[idx].pays, "Broker", false); sauvegarderDonnees(list, nbC); } } else if (sM == 2) { double q; std::cout << " Quantite de RTC a liquider : "; std::cin >> q; if(list[idx].retroCoins >= q && q > 0) { list[idx].retroCoins -= q; list[idx].soldeCourantUSD += (q * coursRetroCoin); ajouterTransaction(list[idx], "SELL RTC", q, "RTC", list[idx].pays, "Broker", false); sauvegarderDonnees(list, nbC); } } attendreRetour(); }

                else if (chC == 11) { ecranBleu(); std::cout << " === FINANCES DE LEVIER MARGE ===\n 1. Ouvrir Long Levier 10x | 2. Clore la position | 0.Annuler : "; int lM; std::cin >> lM; if(lM == 1 && list[idx].taillePositionRTC == 0) { double m; std::cout << " Fond de garantie / Collateral a deposer (USD) : "; std::cin >> m; if(m > 0 && list[idx].soldeCourantUSD >= m) { list[idx].soldeCourantUSD -= m; list[idx].capitalLevierUSD = m; list[idx].prixEntreeLevierRTC = coursRetroCoin; list[idx].taillePositionRTC = (m * 10.0) / coursRetroCoin; sauvegarderDonnees(list, nbC); std::cout << " Position speculative ouverte avec un levier de 10x.\n"; } } else if(lM == 2 && list[idx].taillePositionRTC > 0) { double pnl = list[idx].taillePositionRTC * (coursRetroCoin - list[idx].prixEntreeLevierRTC); list[idx].soldeCourantUSD += (list[idx].capitalLevierUSD + pnl); list[idx].capitalLevierUSD = 0; list[idx].taillePositionRTC = 0; list[idx].prixEntreeLevierRTC = 0; sauvegarderDonnees(list, nbC); std::cout << " Position close. Solde ajuste.\n"; } attendreRetour(); }
                else if (chC == 12) { afficherGraphiqueSanteClient(list[idx]); attendreRetour(); }
                else if (chC == 13) { ecranBleu(); std::cout << " === AUTOMATE TRADING ===\n Declencher sous (USD) : "; std::cin >> list[idx].declencheurAchatRTC; std::cout << " Volume RTC : "; std::cin >> list[idx].volumeAchatRTC; sauvegarderDonnees(list, nbC); std::cout << " Ordre a seuil memorise.\n"; attendreRetour(); }
                else if (chC == 14) { ecranBleu(); std::cout << " === CARTE BANCAIRE VIRTUELLE ===\n Num : " << list[idx].carte.numero << "\n Type : " << list[idx].carte.typeCode << "\n Plafond Hebdo : " << list[idx].carte.plafondHebdo << " USD\n Utilise : " << list[idx].carte.depenseCouranteHebdo << " USD\n Active : " << (list[idx].carte.active ? "OUI" : "NON") << "\n 1. Commuter statut : "; int optC; std::cin >> optC; if(optC == 1) list[idx].carte.active = !list[idx].carte.active; sauvegarderDonnees(list, nbC); attendreRetour(); }
                else if (chC == 15) { exporterRecu(list[idx]); attendreRetour(); }
            }
            effetMatrix();
        }
    }
    std::cout << "\n Infrastructure BANK-GLODY-CLAVER desactivee avec succes. Deconnexion.\n"; 
    return 0;
}


