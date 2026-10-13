#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "nettoyage.h"
#include "traduction.h"
#include "simulation.h"


int main(int argc, char * argv[]){

	// ouverture du fichier texte à traduire
	if (argc < 2){
		
		printf("ERREUR : il manque un argument\n");
		return 0;
	
	}
	else if (argc > 2){
	
		printf("ERREUR : il y a trop d'arguments\n");
		return 0;
	
	}
	
	// partie nettoyage (et gestion des étiquettes en début de ligne)
	int t1 = nettoyage(argc, argv);
	if (!t1){ 
	
		libereEtiq();
		return 0;
	}
	
	// partie traduction (et gestion des sauts vers étiquettes)
	FILE *fIn = fopen("copie.txt", "r");
    if (!fIn) {
        printf("Erreur d'ouverture du fichier copie.txt à traduire\n");
        libereEtiq();
        return 0;
    }
    
    FILE *fOut = fopen("hexa.txt", "w");
    if (!fOut) {
        printf("Erreur de création du fichier hexa.txt\n");
        fclose(fIn);
        libereEtiq();
        return 0;
    }
    
    char ligne[TAILLE_MAX_L];
    int num_ligne = 0;
    while (fgets(ligne, sizeof(ligne), fIn)) {
    
    	num_ligne++;
    	
    	// on ne traduit pas les lignes vides
    	if (estLigneVide(ligne)){
    		continue;
    	}
    	
    	char trad_hex[9];
    	if(!traduction(ligne, sizeof(trad_hex), trad_hex)) {
    		
    		printf("Ligne concernée par l'erreur: ligne %d: %s\n", num_ligne, ligne);
    		fclose(fIn);
            fclose(fOut);
            libereEtiq();
            return 0;
    	
    	}
    	
    	// si on n'a pas rencontré de problème on enregistre dans hexa.txt et passe à la ligne suivante
    	fprintf(fOut, "%s\n", trad_hex);
    
    }
    
    fclose(fIn);
    fclose(fOut);
    libereEtiq();
    
	// partie simulation 
	Machine m;
    machine_zero(&m);
    srand(time(NULL));
	
	// chargement du fichier hexa.txt en mémoire 
    if (!charger_hexa_txt(&m, "hexa.txt")) {
        printf("ERREUR : impossible de charger le fichier hexa.txt en mémoire");
        return 1;
    }
    
    // boucle de simulation 
    while (execute_instruction(&m)) { }
    printf("Fin simulation. PC final=0x%04X\n", m.PC);
	
	return 0;

}