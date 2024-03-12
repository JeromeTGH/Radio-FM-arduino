/*
   ______               _                  _///_ _           _                   _
  /   _  \             (_)                |  ___| |         | |                 (_)
  |  [_|  |__  ___  ___ _  ___  _ __      | |__ | | ___  ___| |_ _ __ ___  _ __  _  ___  _   _  ___
  |   ___/ _ \| __|| __| |/ _ \| '_ \_____|  __|| |/ _ \/  _|  _| '__/   \| '_ \| |/   \| | | |/ _ \
  |  |  | ( ) |__ ||__ | | ( ) | | | |____| |__ | |  __/| (_| |_| | | (_) | | | | | (_) | |_| |  __/
  \__|   \__,_|___||___|_|\___/|_| [_|    \____/|_|\___|\____\__\_|  \___/|_| |_|_|\__  |\__,_|\___|
                                                                                      | |
                                                                                      \_|
  Fichier :       prgRadioFMarduino.ino  
  Description :   Programme permettant de réaliser une simple radio FM

  Licence :       BY-NC-ND 4.0 CC (https://creativecommons.org/licenses/by-nc-nd/4.0/deed.fr)
  
  Remarques :     - l'arduino utilisé ici sera un Arduino Nano
                  - la réception radio sera assurée par un module TEA5767 (module piloté en i2c)
                  - l'amplification sonore sera assurée par un module PAM8403
                  - un afficheur oled 0,96" (contrôleur SSD1306) permettra l'affichage des infos (afficheur piloté en i2c)
                  - un sélecteur rotatif EC11 permettra à l'utilisateur d'interagir avec cette radio FM

  Dépôt GitHub :  https://github.com/JeromeTGH/Radio-FM-arduino (fichiers sources du projet)

  Auteur :        Jérôme TOMSKI (https://passionelectronique.fr/)
  Créé le :       12.03.2024
  
*/


// Inclusion des librairies utilisées ici
#include <Adafruit_SSD1306.h>                   // Source : https://github.com/adafruit/Adafruit_SSD1306

// Définition des broches de raccordement Arduino Nano → Encodeur rotatif EC11
#define broche_SW_encodeur_rotatif      2       // Entrée D2 de l'Arduino reliée à la broche SW      de l'encodeur rotatif EC11
#define broche_A_encodeur_rotatif       3       // Entrée D3 de l'Arduino reliée à la broche A (CLK) de l'encodeur rotatif EC11
#define broche_B_encodeur_rotatif       4       // Entrée D4 de l'Arduino reliée à la broche B (DT)  de l'encodeur rotatif EC11

// Définition des paramètres de l'écran OLED
#define nombreDePixelsEnLargeur         128     // Taille de l'écran OLED, en pixel, au niveau de sa largeur
#define nombreDePixelsEnHauteur         64      // Taille de l'écran OLED, en pixel, au niveau de sa hauteur
#define brocheResetEcranOLED            -1      // Reset de l'OLED partagé avec l'Arduino (d'où la valeur à -1, et non un numéro de pin)
#define adresseI2CecranOLED             0x3C    // Adresse de "mon" écran OLED sur le bus i2c (généralement égal à 0x3C ou 0x3D)

// Instanciation des classes
Adafruit_SSD1306 ecranOLED(nombreDePixelsEnLargeur, nombreDePixelsEnHauteur, &Wire, brocheResetEcranOLED);

// Variables
int etatPrecedentLigneSW;           // Cette variable nous permettra de stocker le dernier état de la ligne SW, afin de le comparer à l'actuel
int etatPrecedentLigneA;            // Cette variable nous permettra de stocker le dernier état de la ligne A (CLK), afin de le comparer à l'actuel
int etatPrecedentLigneB;            // Cette variable nous permettra de stocker le dernier état de la ligne B (DT), afin de le comparer à l'actuel


// ========================
// Initialisation programme
// ========================
void setup() {

  // Configuration des entrées utilisées sur l'Arduino Nano (avec activation des pull-ups internes, au passage)
  pinMode(broche_SW_encodeur_rotatif, INPUT_PULLUP);
  pinMode(broche_A_encodeur_rotatif,  INPUT_PULLUP);
  pinMode(broche_B_encodeur_rotatif,  INPUT_PULLUP);

  // Configuration de la LED embarquée sur l'Arduino Nano (celle qui sert à l'exemple "blink", couramment)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Initialisation de l'écran OLED
  if(!ecranOLED.begin(SSD1306_SWITCHCAPVCC, adresseI2CecranOLED)) {
    while(1) {
      // Arrêt du programme (boucle infinie, en faisant clignoter la led "blink" de l'Arduino Nano)
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
    }
  }

  // Affichage de l'écran OLED
  ecranOLED.clearDisplay();                     // Effaçage de l'intégralité du buffer
  ecranOLED.setTextSize(2);                     // Définit la taille du texte (1, 2, 3, ...)
  ecranOLED.setCursor(0, 0);                    // Déplacement du curseur depuis l'angle supérieur gauche (0 pixels vers la droite, et 0 pixels vers le bas)
  ecranOLED.setTextColor(SSD1306_BLACK, SSD1306_WHITE);   // Couleur du texte = en "noir" sur fond "blanc" (bleu ou jaune, plutôt, compte tenu de l'écran utilisé)
  ecranOLED.println(" Radio FM ");              // Écriture du texte en mémoire buffer
  ecranOLED.display();                          // Transfert du buffer à l'écran

  // Petite pause pour laisser le temps aux signaux de se stabiliser
  delay(200);

  // Mémorisation des valeurs initiales des lignes SW/A/B de l'encodeur rotatif, au démarrage du programme
  etatPrecedentLigneSW = digitalRead(broche_SW_encodeur_rotatif);
  etatPrecedentLigneA  = digitalRead(broche_A_encodeur_rotatif);
  etatPrecedentLigneB  = digitalRead(broche_B_encodeur_rotatif);

  // Activation d'interruptions sur les lignes A et B
  attachInterrupt(digitalPinToInterrupt(broche_A_encodeur_rotatif),  changementSurLigneA,  CHANGE);   // CHANGE => détecte tout changement d'état
  attachInterrupt(digitalPinToInterrupt(broche_SW_encodeur_rotatif), changementSurLigneSW, CHANGE);   // CHANGE => détecte tout changement d'état

  // Passage à la fonction loop
  delay(500);
  
}


// =================
// Boucle principale
// =================
void loop() {
  // Aucun code, pour l'instant
  
}


// ============================================
// Routine d'interruption : changementSurLigneA
// ============================================
void changementSurLigneA() {

}


// =============================================
// Routine d'interruption : changementSurLigneSW
// =============================================
void changementSurLigneSW() {

}
