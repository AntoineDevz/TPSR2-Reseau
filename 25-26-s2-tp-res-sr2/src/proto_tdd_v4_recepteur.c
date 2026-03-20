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
    paquet_t pdata, pack;                  /* paquet utilisé par le protocole */
    int fin = 0;                 /* condition d'arrêt */
    int paquet_attendu=0;
    int taille_fenetre;
    int borne_inf =0;
    int param = atoi(argv[1]);
    if(param<8 && param>0) {
        taille_fenetre=param;
    } else {
        taille_fenetre=4;
    }
    bool buffer_fenetre[16];
    for(int i=0; i<16; i++) {
        buffer_fenetre[i] = false;
    }

    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    

    /* tant que le récepteur reçoit des données */
    while ( !fin ) {

        // attendre(); /* optionnel ici car de_reseau() fct bloquante */
        de_reseau(&pdata);

        if(verifier_somme_controle(pdata)) {
            pack.type = ACK;
            pack.num_seq = pdata.num_seq;
            pack.lg_info = 0;

            /* extraction des donnees du paquet recu */
            for (int i=0; i<pdata.lg_info; i++) {
                message[i] = pdata.info[i];
            }

            
            if(pdata.num_seq == paquet_attendu) {
                buffer_fenetre[pdata.num_seq] = false;
                borne_inf= (borne_inf+1)%16;
                paquet_attendu = (paquet_attendu+1)%16;
                fin = vers_application(message, pdata.lg_info);
            } else {
                pack.num_seq = (paquet_attendu + 15) % 16; //Ici, on envoie un ACK pour le dernier paquet reçu dans la fenêtre (le paquet attendu -1 modulo 16)
            }
            if(dans_fenetre(borne_inf,pdata.num_seq,taille_fenetre) && pdata.num_seq != paquet_attendu) {
                buffer_fenetre[pdata.num_seq] = true;
            }
                pack.somme_ctrl = generer_somme_controle(pack);
        } else {
            pack.type = ACK;
            pack.lg_info = 0;
            pack.num_seq = (paquet_attendu + 15) % 16;
            pack.somme_ctrl = generer_somme_controle(pack);
        }
        vers_reseau(&pack);
    }

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
