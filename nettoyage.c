#include "nettoyage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// tableau dans lequel on stocke toutes les étiquettes valides via des pointeurs
Etiquette * tab[MAX_ET];
size_t t_tab = 0; // correspond au nombre d'étiquettes valides 


int estLigneVide(const char * str){
	
	while(*str) {
	
		// si l'un des caractères n'est ni un espace ni une tabulation ni un retour a la ligne la ligne n'est pas vide
		if (*str != ' ' && *str != '\t' && *str != '\n' && *str !='\r'){
			return 0;
		}
		str++; 
	}
	
	return 1;
}



static char * suppEspaces(char * str){
	
	// supprime uniquement les espaces et tabulations
	while (*str == ' ' || *str == '\t'){
		str++;
	}
	
	//sinon 
	int n = strlen(str);
	char * fin = str + n-1; // pointeur sur le dernier element de str
	
	while (*fin == ' ' || *fin == '\t'){
		fin--;
	}
	
	*(fin + 1) = '\0'; 
	
	return str;

}



static int estLigneEtiq(const char *str, char *nom_etiq){
	
	if (strchr(str, ':')!=NULL){
		
		int i;
		for(i=0; i<TAILLE_MAX_ET && str[i] != ':'; i++){
			
			nom_etiq[i] = str[i];
		
		}
		
		// si i est nul cela signifie que l'étiquette devant : est vide, c'est une erreur, on renvoie 3
		if (i==0){
		
			return 3;
		}
		
		// si on arrive ici on a trouvé une étiquette potentiellement valide
		else if (str[i] == ':'){
			
			nom_etiq[i] = '\0';
			return 1;
		
		}
		
		// on renvoie 2 quand la ligne possède une étiquette mais que son nom est trop long
		return 2;
	
	}
	
	return 0;

}



int estDansTab(const char *nom){
	
	if (!nom) return 0;
	
	for(size_t i=0; i < t_tab; i++){
	
		if (tab[i] && strcmp(tab[i]->nom, nom) == 0){
			
			return 1;
		
		}
	
	}
	
	return 0;
}



static int estEtiqValide(const char *nom_etiq){
	
	size_t n = strlen(nom_etiq);
	
	for(int i=0; nom_etiq[i]; i++){
		
		if(!isalnum((unsigned char)nom_etiq[i])){
			
			return 0;
		
		}
	}
	
	// si on arrive ici le nom est uniquement composé de lettres et / ou de chiffres
	
	if (nom_etiq[0] == 'r'){
		if (n==2 && isdigit((unsigned char)nom_etiq[1])) return 2;
		
		else if (n==3 && isdigit((unsigned char)nom_etiq[1]) && isdigit((unsigned char)nom_etiq[2])){ 
		
			int c = atoi((nom_etiq+1)); // atoi renvoie toujours un entier positif 
			if (c < 32){
				return 2;
			}	
	
		}
	}
	
	else if (nom_etiq[0] == 'h' && n>1){
		int i;
		for(i=1; nom_etiq[i]; i++){
		
			if(!isxdigit((unsigned char)nom_etiq[i])){
				break;
			}
		
		}
		
		// si on a atteint la fin du for : le nom de l'étiquette est un nombre en hexadécimal
		if (nom_etiq[i] == '\0') {
			return 3;
		}
	
	}
	
	else { // on regarde si le nom de l'étiquette n'est composée que de chiffres
		int i;
		for(i=0; nom_etiq[i]; i++){
		
			if(!isdigit((unsigned char)nom_etiq[i])){
				break;
			}
		}
		
		// si on a atteint la fin du for : le nom de l'étiquette est un nombre
		if (nom_etiq[i] == '\0') {
			return 3;
		}
	}
	
	// si le nom de l'étiquette existe déjà dans tab alors l'étiquette n'est pas unique
	if (estDansTab(nom_etiq)) return 4;
	
	return 1;
}



static char * suppEtiq(char *str){

	// supprime l'étiquette en début de ligne et renvoie la ligne sans l'étiquette ni les espaces entre : et le contenu de la ligne
	int i;
	for(i=0; str[i] != ':'; i++);
	i++;
	char * lres = suppEspaces(str+i);
	return lres;
	

}


int trouveAdr(const char *nom_etiq){
	
	if (!nom_etiq) return -1;
	
	size_t i=0;
	while (i<t_tab && tab[i] && strcmp(tab[i]->nom, nom_etiq) != 0){
		i++;
	}
	
	if (i<t_tab && tab[i]) {
		return tab[i]->adr;
	}
	
	return -1;
} 



void libereEtiq(void){
	
	for(size_t i = 0; i < t_tab; i++){
		
		free(tab[i]);
		
	}
	
	t_tab = 0;
}



int nettoyage(int argc, char * argv[]){
	
	// lecture du fichier texte contenant les instructions assembleur
	FILE * f = fopen(argv[1], "r");
	if (!f){
		printf("Erreur d'ouverture du fichier à traduire\n");
		return 0;
	}
	
	char ligne[TAILLE_MAX_L];
	int num_ligne = 0;
	int adr_courante = 0;
	
	//premier parcours pour enregistrer les etiquettes dans le tableau global tab 
	while (fgets(ligne, sizeof(ligne), f)) {
		
		num_ligne++;
		
		if (estLigneVide(ligne)){
			continue;
		}
		
		char * copie_ligne = suppEspaces(ligne);
		char nom_etiq[TAILLE_MAX_ET];
		int t;
		
		// si on arrive ici la ligne n'est pas vide 
		
		// si la ligne n'a pas d'étiquette on passe à la ligne suivante 
		if (!(t = estLigneEtiq(copie_ligne, nom_etiq))){
			
			adr_courante += 4;
			continue;
		}
	
		// si on arrive ici la ligne a forcément une étiquette au début
		else{
			
			if (t==2){
				printf("Erreur d'étiquette à la ligne %d : le nom de l'étiquette doit faire 63 caractères ou moins\n", num_ligne);
				return 0;
			}
			
			if (t==3){
				printf("Erreur d'étiquette à la ligne %d : le nom de l'étiquette ne peut pas être vide\n", num_ligne);
				return 0;
			}
			
			int tn = estEtiqValide(nom_etiq);
			
			if (!tn){
				printf("Erreur d'étiquette à la ligne %d : le nom de l'étiquette doit être composé seulement de lettres et chiffres\n", num_ligne);
				return 0;
			}
			
			if (tn==2){
				printf("Erreur d'étiquette à la ligne %d : le nom de l'étiquette ne peut pas être un nom de registre\n", num_ligne);
				return 0;
			}
			
			if (tn==3){
				printf("Erreur d'étiquette à la ligne %d : le nom de l'étiquette ne peut pas être un nombre (décimal ou hexadécimal)\n", num_ligne);
				return 0;
			}
			
			if (tn==4){
				printf("Erreur d'étiquette à la ligne %d : le nom de l'étiquette doit être unique\n", num_ligne);
				return 0;
			}
			
			// si on arrive ici l'étiquette est valide, on créée un pointeur sur une structure Etiquette et on l'ajoute dans tab
			Etiquette * ptr = malloc(sizeof(Etiquette));
			if (!ptr) {
    			printf("Erreur d'allocation mémoire\n");
				return 0;
			}
			
			strcpy(ptr->nom, nom_etiq); 
			ptr->adr = adr_courante;
			tab[t_tab] = ptr;
			t_tab++;
			adr_courante += 4;
		
		}
	}
	
	// On vérifie que toutes les instructions puissent être stockées en mémoire (adressée de 0 à 65 535 et chaque instruction prend 4 octets )
	if (adr_courante > 65536){
		printf("Erreur : votre fichier est trop volumineux, veuillez entrer moins de 16 383 instructions\n");
		return 0;
	}
		
	
	fseek(f, 0, SEEK_SET); // on revient au début du fichier 
	num_ligne = 0;
	FILE * copie = fopen("copie.txt", "w");
	if (!copie){
		printf("Erreur de création du fichier copie\n");
		fclose(f);
		return 0;
	}
	

	// deuxième parcours pour copier les instructions à traduire dans copie.txt, supprimer les étiquettes en début de ligne
	while (fgets(ligne, sizeof(ligne), f)) {
	
		num_ligne++;
		
		if (estLigneVide(ligne)){
			// on recopie dans copie.txt et passe à la ligne suivante de f
			fputs(ligne, copie);
			continue;
		}
		
		char * copie_ligne = suppEspaces(ligne);
		char nom_etiq[TAILLE_MAX_ET];
	
		// si la ligne a une étiquette devant, cette dernière est forcément valide grâce aux tests effectués dans la boucle précédente
		if (estLigneEtiq(copie_ligne, nom_etiq)){
			
			
			// il faut modifier la ligne pour retirer l'étiquette devant ainsi que les espaces au debut et fin de la ligne sans étiquette 
			copie_ligne = suppEtiq(copie_ligne);
			
			//on recopie la ligne sans l'étiquette et passe à la ligne suivante 
			fputs(copie_ligne, copie);

		}
		
		else{
		
			fputs(copie_ligne, copie);
		
		}
		
		

	}
	
	fclose(f);
	fclose(copie);
	return 1;
	
}

