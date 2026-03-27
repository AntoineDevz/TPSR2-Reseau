/*************************************************************
* proto_tdd_v0 -  récepteur                                  *
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
#include <stdbool.h>
#include <stdlib.h>

/* =============================== */
/* Programme principal - récepteur */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message pour l'application */
    paquet_t pdata, pack;          /* paquet utilisé par le protocole */
    paquet_t buffer_paquets[16];
    int fin = 0;                   /* condition d'arrêt */
    int paquet_attendu = 0;
    int taille_fenetre;
    int param = (argc > 1) ? atoi(argv[1]) : 0;
    if (param > 0 && param <= 8) {
        taille_fenetre = param;
    } else {
        taille_fenetre = 4;
    }
    bool buffer_fenetre[16];
    for (int i = 0; i < 16; i++) {
        buffer_fenetre[i] = false;
    }

    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    

    /* tant que le récepteur reçoit des données */
    while ( !fin ) {

        // attendre(); /* optionnel ici car de_reseau() fct bloquante */
        de_reseau(&pdata);

        if (pdata.type == DATA && verifier_somme_controle(pdata)) {
            int borne_inf_prec = (paquet_attendu + 16 - taille_fenetre) % 16;

            if (dans_fenetre(paquet_attendu, pdata.num_seq, taille_fenetre)) {
                /* Ack de tout paquet valide dans la fenêtre de réception */
                pack.type = ACK;
                pack.num_seq = pdata.num_seq;
                pack.lg_info = 0;
                pack.somme_ctrl = generer_somme_controle(pack);
                vers_reseau(&pack);

                /* Bufferisation des paquets en/sous séquence */
                if (!buffer_fenetre[pdata.num_seq]) {
                    buffer_paquets[pdata.num_seq] = pdata;
                    buffer_fenetre[pdata.num_seq] = true;
                }

                /* Livraison en ordre dès que possible */
                while (buffer_fenetre[paquet_attendu] && !fin) {
                    paquet_t p = buffer_paquets[paquet_attendu];
                    for (int i = 0; i < p.lg_info; i++) {
                        message[i] = p.info[i];
                    }
                    fin = vers_application(message, p.lg_info);
                    buffer_fenetre[paquet_attendu] = false;
                    paquet_attendu = (paquet_attendu + 1) % 16;
                }
            } else if (dans_fenetre(borne_inf_prec, pdata.num_seq, taille_fenetre)) {
                /* Doublon d'un paquet déjà reçu : on réémet son ACK */
                pack.type = ACK;
                pack.num_seq = pdata.num_seq;
                pack.lg_info = 0;
                pack.somme_ctrl = generer_somme_controle(pack);
                vers_reseau(&pack);
            }
        }
    }

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
