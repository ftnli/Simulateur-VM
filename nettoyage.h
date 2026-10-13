#ifndef NETTOYAGE_H
#define NETTOYAGE_H

#define TAILLE_MAX_ET 64 // pour la structure 
#define MAX_ET 16384
#define TAILLE_MAX_L 256

#include <stddef.h> // pour size_t

typedef struct {
	
	char nom[TAILLE_MAX_ET];
	int adr;

} Etiquette;

extern Etiquette * tab[MAX_ET]; // extern car utilisés dans un autre fichier .c
extern size_t t_tab;

int estLigneVide(const char * str);

int estDansTab(const char *nom);

int trouveAdr(const char *nom_etiq);

void libereEtiq(void);

int nettoyage(int argc, char * argv[]);

#endif	