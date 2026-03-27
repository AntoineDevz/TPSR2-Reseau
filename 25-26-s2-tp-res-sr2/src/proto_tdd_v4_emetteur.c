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
    paquet_t pack;                   /* paquet utilisé par le protocole */
    paquet_t tab_p[16];
    int evenement;
    int borne_inf = 0, curseur = 0;
    int taille_fenetre;
    int param = (argc > 1) ? atoi(argv[1]) : 0;
    if (param > 0 && param <= 8) {
        taille_fenetre = param;
    } else {
        taille_fenetre = 4;
    }
    bool ack_recu[16];
    bool envoye[16];
    for (int i = 0; i < 16; i++) {
        ack_recu[i] = false;
        envoye[i] = false;
    }

    init_reseau(EMISSION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    de_application(message, &taille_msg);

    
    /* tant que l'émetteur a des données à envoyer */
    while (taille_msg != 0 || borne_inf != curseur) {
        if (taille_msg > 0 && dans_fenetre(borne_inf, curseur, taille_fenetre)) {
            /* construction et envoi du prochain paquet dans la fenêtre */
            for (int i = 0; i < taille_msg; i++) {
                tab_p[curseur].info[i] = message[i];
            }
            tab_p[curseur].lg_info = taille_msg;
            tab_p[curseur].type = DATA;
            tab_p[curseur].num_seq = curseur;
            tab_p[curseur].somme_ctrl = generer_somme_controle(tab_p[curseur]);

            vers_reseau(&tab_p[curseur]);

            envoye[curseur] = true;
            ack_recu[curseur] = false;
            depart_temporisateur_num(curseur, 70);

            curseur = (curseur + 1) % 16;
            de_application(message, &taille_msg);
        
        } else {
            evenement = attendre();
            if (evenement == PAQUET_RECU) {
                de_reseau(&pack);

                if (pack.type == ACK && verifier_somme_controle(pack) &&
                    dans_fenetre(borne_inf, pack.num_seq, taille_fenetre)) {

                    if (!ack_recu[pack.num_seq]) {
                        ack_recu[pack.num_seq] = true;
                        arret_temporisateur_num(pack.num_seq);
                    }

                    /* Décalage de fenêtre uniquement tant que les ACK sont contigus depuis borne_inf */
                    while (borne_inf != curseur && ack_recu[borne_inf]) {
                        ack_recu[borne_inf] = false;
                        envoye[borne_inf] = false;
                        borne_inf = (borne_inf + 1) % 16;
                    }
                }
            } else {
                int seq_timeout = evenement;

                /* Selective Repeat : retransmission du seul paquet expiré */
                if (envoye[seq_timeout] && !ack_recu[seq_timeout] &&
                    dans_fenetre(borne_inf, seq_timeout, taille_fenetre)) {
                    vers_reseau(&tab_p[seq_timeout]);
                    depart_temporisateur_num(seq_timeout, 70);
                }
            }
        }
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
