#include <Arduino.h>
#include <Servo.h> 

// --- Configuration Moteurs (Fixée par la carte Romeo) ---
const int E1 = 5; // Vitesse Gauche
const int M1 = 4; // Direction Gauche
const int E2 = 6; // Vitesse Droit
const int M2 = 7; // Direction Droit

// --- Configuration Servomoteur ---
Servo monServo;
const int pinServo = 2;
const int origine = 830;  // Position de repos (à ajuster selon le montage)
const int cible = 1830;   // Position d'action (à ajuster selon le montage)

// Variables pour le contrôle non-bloquant du servomoteur
bool servoEnMarche = false;
unsigned long chronometreServo = 0; 
int etapeServo = 0; // 0: Repos, 1: En position cible, 2: Retour origine

// --- État du système ---
char modeCourant = 'M'; // 'M' = Manuel, 'B' = Balle (Automatique)

// Déclarations (Requises par PlatformIO)
void avancerToutDroit(int vitesse);
void reculer(int vitesse);
void tournerGauche(int vitesse);
void tournerDroite(int vitesse);
void stopper();

void setup() {
  pinMode(M1, OUTPUT);
  pinMode(E1, OUTPUT);
  pinMode(M2, OUTPUT);
  pinMode(E2, OUTPUT);

  monServo.attach(pinServo);
  monServo.writeMicroseconds(origine); 

  // 115200 bauds obligatoire pour la communication BLE DFRobot
  Serial.begin(115200); 
}

void loop() {
  // 1. Contrôle matériel (Broche A7)
  int bouton = analogRead(7);
  if (bouton < 50) {
    servoEnMarche = true;
  }
  else if (bouton > 100 && bouton < 250) {
    // Arrêt d'urgence du mécanisme
    servoEnMarche = false;
    etapeServo = 0;
    monServo.writeMicroseconds(origine); 
  }

  // 2. Réception Bluetooth
  if (Serial.available() > 0) {
    char commande = Serial.read();

    // Sélection du mode ou actionnement du bras ('C')
    if (commande == 'M' || commande == 'm') { modeCourant = 'M'; stopper(); } 
    else if (commande == 'B' || commande == 'b') { modeCourant = 'B'; }
    else if (commande == 'C' || commande == 'c') { 
      servoEnMarche = !servoEnMarche; 
      if (!servoEnMarche) {
        etapeServo = 0;
        monServo.writeMicroseconds(origine);
      }
    }

    // Commandes de pilotage manuel
    if (modeCourant == 'M') {
      switch (commande) {
        case 'Z': case 'z': avancerToutDroit(200); break;
        case 'S': case 's': reculer(200); break;
        case 'Q': case 'q': tournerGauche(150); break; 
        case 'D': case 'd': tournerDroite(150); break;
        case 'X': case 'x': case ' ': stopper(); break;
      }
    }
  }

  // 3. Séquence Automatique (Récupération Balle)
  if (modeCourant == 'B') {
    stopper();
    delay(1000); // Temps d'attente avant démarrage
    
    avancerToutDroit(255); 
    delay(800); // Distance de déplacement à calibrer
    
    stopper(); 
    modeCourant = 'M'; // Retour au mode manuel après la séquence
  }

  // 4. Gestion asynchrone du Servomoteur
  // Utilisation de millis() pour maintenir l'écoute Bluetooth active
  if (servoEnMarche) {
    if (etapeServo == 0) {
      monServo.writeMicroseconds(cible);
      chronometreServo = millis(); 
      etapeServo = 1;
    }
    else if (etapeServo == 1) {
      // Maintien de la position cible pendant 2000 ms
      if (millis() - chronometreServo >= 2000) {
        monServo.writeMicroseconds(origine);
        chronometreServo = millis(); 
        etapeServo = 2;
      }
    }
    else if (etapeServo == 2) {
      // Pause de 2000 ms avant le prochain cycle
      if (millis() - chronometreServo >= 2000) {
        etapeServo = 0;
      }
    }
  }
}

// --- Contrôle des Moteurs ---
void avancerToutDroit(int vitesse) {
  digitalWrite(M1, LOW); digitalWrite(M2, LOW);
  analogWrite(E1, vitesse); analogWrite(E2, vitesse);
}
void reculer(int vitesse) {
  digitalWrite(M1, HIGH); digitalWrite(M2, HIGH);
  analogWrite(E1, vitesse); analogWrite(E2, vitesse);
}
void tournerGauche(int vitesse) {
  digitalWrite(M1, HIGH); digitalWrite(M2, LOW);  
  analogWrite(E1, vitesse); analogWrite(E2, vitesse);
}
void tournerDroite(int vitesse) {
  digitalWrite(M1, LOW);  digitalWrite(M2, HIGH); 
  analogWrite(E1, vitesse); analogWrite(E2, vitesse);
}
void stopper() {
  analogWrite(E1, 0); analogWrite(E2, 0);
}