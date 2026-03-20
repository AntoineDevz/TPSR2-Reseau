/*************************************************************
* proto_tdd_v0 -  émetteur                                   *
* TRANSFERT DE DONNEES  v0                                   *
*                                                            *
* Protocole sans contrôle de flux, sans reprise sur erreurs  *
*                                                            *
* Université de Toulouse / FSI / Dpt d'informatique          *
**************************************************************/

#include <stdio.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"
#include <stdlib.h>
#define MAXTRANSMISSION 100

/* =============================== */
/* Programme principal - émetteur  */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg;                  /* taille du message */
    paquet_t pack;                  /* paquet utilisé par le protocole */
    paquet_t tab_p[16];
    int evenement; 
    int borne_inf = 0, curseur=0;
    int taille_fenetre;
    int param = atoi(argv[1]);
    if(param<8 && param>0) {
        taille_fenetre=param;
    } else {
        taille_fenetre=4;
    }
    bool buffer_fenetre[16];
    for(int i=0; i<16; i++) {
        buffer_fenetre[i] = false;
    } // On initialise le buffer de la fenêtre à false (pas de paquet envoyé)

    init_reseau(EMISSION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    de_application(message, &taille_msg);

   
    /* tant que l'émetteur a des données à envoyer */
    while ( taille_msg != 0  || borne_inf != curseur) {
        if(dans_fenetre(borne_inf,curseur,taille_fenetre) && (taille_msg>0)) {
              /* construction paquet */
              for (int i=0; i<taille_msg; i++) {
                tab_p[curseur].info[i] = message[i];
            }
            tab_p[curseur].lg_info = taille_msg;
            tab_p[curseur].type = DATA;
            tab_p[curseur].num_seq = curseur;
            tab_p[curseur].somme_ctrl = generer_somme_controle(tab_p[curseur]);
            vers_reseau(&tab_p[curseur]);
            depart_temporisateur_num(curseur,70);
            buffer_fenetre[curseur] = false; // On reinitialise la case du buffer de la fenêtre à false (paquet envoyé, on attend l'ACK)
        

            curseur = (curseur + 1) % 16; // Incrémentation du curseur (+1 modulo n, en l'occurence n=16 ici)
        /* lecture de donnees provenant de la couche application */
        de_application(message, &taille_msg);
        
        } else { // Reception d'un ACK
            evenement = attendre();
            if (evenement == PAQUET_RECU) {
                de_reseau(&pack);
                if (pack.type == ACK && verifier_somme_controle(pack) && dans_fenetre(borne_inf, pack.num_seq, taille_fenetre) ){
                    
                    while(buffer_fenetre[borne_inf]){
                        borne_inf = (pack.num_seq + 1) % 16;
                    }
                    arret_temporisateur_num(pack.num_seq);
                    buffer_fenetre[pack.num_seq]=true; //Si on a bien reçu un paquet hors séquence valide, on met la case du buffer de la fenêtre correspondante à true (ACK reçu)

                }
            } else {
                vers_reseau(&tab_p[evenement]);
                depart_temporisateur_num(evenement,70);
            }
        }
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
