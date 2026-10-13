#include "traduction.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


const char *ope[32]={"or", "and", "xor", "add", "sub", "mul", "div", "shr", "0","0","0","0",
"ldb", "ldh", "ldw", "stb", "sth", "stw", "0","0","0", "jmp", "jzs", "jzc","jcs", "jcc", "jns",
"jnc", "in", "out", "rnd", "hlt"}; 

typedef struct error{

    const char * MESSAGE_ERROR;
    
} error;

void PRINT_ERROR(error err){
    printf("ERREUR : %s\n", err.MESSAGE_ERROR);
}

error OP_INCONNUE = {"Vous utilisez un opérateur inconnu à notre système."};
error NB_ARGS_INVALIDE = {"Le nombre d'arguments fournis est invalide."};
error ENTREE_TROP_LONGUE = {"L'entrée fournie est trop longue."};   
error REGISTRE_INVALIDE = {"Le registre fourni est invalide."};
error IMM_INVALIDE = {"La valeur immédiate fournie est invalide."}; 
error SORTIE_TROP_PETITE = {"La taille de la sortie est trop petite pour contenir l'instruction."};
error SYNTAXE_INVALIDE = {"La syntaxe de l'instruction est invalide."};
error ETIQ_INVALIDE = {"L'étiquette n'est pas définie."};


static int indice_operande(char elt[6]){//fonction pour avoir l'indice de l'élément s'il est présent dans un tableau, et return -1 si l'élément n'est pas présent
    for (int i=0; i<32; i++){
        if (strcmp(ope[i], elt)==0){
            return i;
        }
    }
    return -1;
}

// FONCTIONS POUR GERER LES BETISES DE L'UTILISATEUR

static int estSuiteDeChiffres(const char * str) { //Vérifie si la chaîne de caractères passée en argument est non-vide, et composée uniquement de chiffres décimaux.
    if (!str ||*str == '\0') {
        return 0; 
    }
    while (*str) {
        if (!isdigit((unsigned char)*str)) { //unsigned char pour éviter les problèmes (param de isdigit est int)
            return 0; 
        }
        str++;
    }
    return 1; 
}

static int estHexa(const char * str) {
    if (!str || *str == '\0') {
        return 0;
    }
    while (*str) {
        if (!isxdigit((unsigned char)*str)) { 
            return 0; 
        }
        str++;
    }
    return 1; 
}



static int lecture_et_syntaxe_registre(char *chaine, int *reg){ //Vérifie le registre: refuse r3x..., r, r-1, etc
    if(!chaine||chaine[0]!='r'){
        PRINT_ERROR(REGISTRE_INVALIDE);
        return 0;}
    if(!estSuiteDeChiffres(chaine+1)){
        PRINT_ERROR(REGISTRE_INVALIDE);
        return 0;}
    int reg_num = atoi(chaine+1);
    if(reg_num<0 || reg_num>31){
        PRINT_ERROR(REGISTRE_INVALIDE);
        return 0;}
    *reg=reg_num;
    return 1;
}

static int lecture_et_syntaxe_saut(char *chaine, int *val){ //Vérifie l'adresse d'un saut sous forme de u16 = unsigned char sur 16 bits: refuse 3x..., -1, 70000, etc
    if(!chaine){
        PRINT_ERROR(IMM_INVALIDE);
        return 0;}

    int val_num;

    if(chaine[0]=='h'){
        if(!estHexa(chaine+1)){
            PRINT_ERROR(IMM_INVALIDE);
            return 0;}
        val_num = (int)strtol(chaine+1, NULL, 16); //strtol est plus pratique que atoi pour la base 16
        }
    else {
        if(!estSuiteDeChiffres(chaine)){
            PRINT_ERROR(IMM_INVALIDE);
            return 0;}
        val_num=atoi(chaine);
    }
    if(val_num<0 || val_num>65535){
        PRINT_ERROR(IMM_INVALIDE);
        return 0;}
    *val=val_num;
    return 1;

}

static int lecture_et_syntaxe_imm(const char *chaine, int *val, int *imm){ //Vérifie la imm: refuse #3x..., #, #-1, #70000, etc
    if(!chaine || chaine[0]!='#'){
        PRINT_ERROR(IMM_INVALIDE);
        return 0;}
    
    if (chaine[1]=='h'){ 
        if(!estHexa(chaine+2)){
            PRINT_ERROR(IMM_INVALIDE);
            return 0;}
        long val_hex = strtol(chaine+2, NULL, 16);
        if(val_hex<0 || val_hex>65535){
            PRINT_ERROR(IMM_INVALIDE);
            return 0;}
        *val = (int)val_hex;
        *imm=1<<16;
    }
    else {
        if(!estSuiteDeChiffres(chaine+1)){
            PRINT_ERROR(IMM_INVALIDE);
            return 0;}
        int val_num = atoi(chaine+1);
        if(val_num<0 || val_num>65535){
            PRINT_ERROR(IMM_INVALIDE);
            return 0;}
        *val=val_num;
        *imm=1<<16;
    }
    return 1;
}

static int lecture_rkS(char *chaine, int *rk, int *S, int *imm){ //va lire et séparer (rn)S ou (rd)S
    if(!chaine||chaine[0]!='('){
        PRINT_ERROR(SYNTAXE_INVALIDE);
        return 0;}

    int i=1, j=0;
    char rk_str[4], S_str[64]; 

    while ((chaine[i]!=')') &&(chaine[i]!='\0')){
        if (i-1>=3){
            PRINT_ERROR(REGISTRE_INVALIDE);
            return 0;}

        rk_str[i-1]=chaine[i];
        i++;
    }
    if(chaine[i]!=')'){
        PRINT_ERROR(SYNTAXE_INVALIDE);
        return 0;}

    rk_str[i-1]='\0';
    i++;
    
    while(chaine[i]!='\0'){
        if(j>=63){
            PRINT_ERROR(ENTREE_TROP_LONGUE); 
            return 0;}
        S_str[j++]=chaine[i++];
    }
    S_str[j]='\0';

    if(!lecture_et_syntaxe_registre(rk_str, rk)){
        return 0;
    }
    if (S_str[0]=='r'){
        if(!lecture_et_syntaxe_registre(S_str, S)){
            return 0;
        }
        *imm = 0;
        *S = (*S) & 0x1F;
    }
    else if(S_str[0]=='#'){
        if(!lecture_et_syntaxe_imm(S_str, S, imm)){
            return 0;
        }
        *S = (*S) & 0xFFFF;
    }
    else {
        PRINT_ERROR(SYNTAXE_INVALIDE);
        return 0;
    }
    return 1;
}

// TRADUCTION


/*Découpe une ligne assembleur en plusieurs "mots" (voir doc) séparés par des espaces ou des virgules.
Elle détecte également les erreurs liées aux virgules.
Remplit le buffer out[5][64] et retourne le nombre de "mots" trouvés. */
static int cut(const char *ligne, char out[5][64]){ 

    if (estLigneVide(ligne) || ligne[0]=='\0' || ligne[0]=='\n') { //estLigneVide provient de nettoyage.c
        return 0;
    }

    int i = 0, j = 0, k = 0;

    // skip espaces/tabs
    while(ligne[i]==' ' || ligne[i]=='\t') i++;

    if(ligne[i]=='\0' || ligne[i]=='\n'){
        return 0;
    }
    if(ligne[i]==','){ //ligne[i] = premier caractère != d'un espace/tab
        PRINT_ERROR(SYNTAXE_INVALIDE);
        return 0;
    }

    // récupération de l'opérateur
    k = 0;
    while(ligne[i] != '\0' && ligne[i] != '\n' &&
          ligne[i] != ' ' && ligne[i] != '\t' && ligne[i] != ','){
        if(k >= 64){
            PRINT_ERROR(ENTREE_TROP_LONGUE);
            return 0;
        }
        out[0][k++] = ligne[i++];
    }
    out[0][k] = '\0';
    j = 1;

    // détection d'une virgule après l'opérateur
    while(ligne[i]==' ' || ligne[i]=='\t') i++;
    if(ligne[i]==','){
        PRINT_ERROR(SYNTAXE_INVALIDE);
        return 0;
    }

    // gestion des opérandes
    while(ligne[i] != '\0' && ligne[i] != '\n'){

        while(ligne[i]==' ' || ligne[i]=='\t') i++;
        if(ligne[i]=='\0' || ligne[i]=='\n') break;

        if(ligne[i]==','){
            PRINT_ERROR(SYNTAXE_INVALIDE); // ",," ou ", \n" => opérande manquant
            return 0;
        }

        if(j > 4){
            PRINT_ERROR(NB_ARGS_INVALIDE);
            return 0;
        }

        k = 0;
        while(ligne[i] != '\0' && ligne[i] != '\n' &&
              ligne[i] != ' ' && ligne[i] != '\t' && ligne[i] != ','){
            if(k >= 32){
                PRINT_ERROR(ENTREE_TROP_LONGUE);
                return 0;
            }
            out[j][k++] = ligne[i++];
        }
        out[j][k] = '\0';
        j++;

        // skip espaces/tabs
        while(ligne[i]==' ' || ligne[i]=='\t') i++;

        // si on n'est pas à la fin, il faut UNE virgule
        if(ligne[i]==','){
            i++; 
            while(ligne[i]==' ' || ligne[i]=='\t'){
                i++;
            }

            // virgule à la fin, ou virgule double
            if(ligne[i]=='\0' || ligne[i]=='\n' || ligne[i]==','){
                PRINT_ERROR(SYNTAXE_INVALIDE);
                return 0;
            }
            continue;
        }
        else if(ligne[i]=='\0' || ligne[i]=='\n'){
            break;
        }
        else {
            // virgule manquante
            PRINT_ERROR(SYNTAXE_INVALIDE);
            return 0;
        }
    }

    return j;
}

static int traduction_arithmetique(char buffer[5][64], int code_ope, int size_out, char *out, int nb_args){
    int rd, rn, S, imm, inst, rd_num, rn_num;

    if(nb_args!=4){
        PRINT_ERROR(NB_ARGS_INVALIDE);
        return 0;
    }

    if (!lecture_et_syntaxe_registre(buffer[1], &rd_num) ||!lecture_et_syntaxe_registre(buffer[2], &rn_num)) {
        return 0;
    }
    rd = rd_num << 22;
    rn = rn_num << 17;
    

    if(buffer[3][0]!='r' && buffer[3][0]!='#'){
        PRINT_ERROR(SYNTAXE_INVALIDE);
        return 0;
    }

    S = atoi(buffer[3]+1);
    if (buffer[3][0]=='r'){ 
        if (!lecture_et_syntaxe_registre(buffer[3], &S)) {
            return 0;
        }
        S = S & 0x1F;
        imm = 0;
    }
    else if(buffer[3][0]=='#'){ 
        if (!lecture_et_syntaxe_imm(buffer[3], &S, &imm)) {
            return 0;
        }
        S = S & 0xFFFF; 
    }
    else {
        PRINT_ERROR(SYNTAXE_INVALIDE);
        return 0;
    }
    inst = (code_ope<<27) | rd | rn | imm | S ;
    if (size_out<9) {
        PRINT_ERROR(SORTIE_TROP_PETITE);
        return 0;
    }
    sprintf(out, "%08X", inst);
    return 1;
}


/*
On gère 3 cas dans cette fonction : si l'opérande du saut est :
- une étiquette, 
- un registre, 
- ou une adresse directe
*/
static int traduction_jmp(char buffer[5][64], int code_ope, int size_out, char *out, int nb_args){ //traduction et détection des erreurs pour les instructions de saut
    int S, imm, inst;

    if(nb_args!=2){
        PRINT_ERROR(NB_ARGS_INVALIDE);
        return 0;
    }
    
    // test étiquettes et gestion de l'adresse 
    if (estDansTab(buffer[1]) != -1){ //estDansTab et trouveAdr proviennent de nettoyage.c
    	
    	int ad = trouveAdr(buffer[1]);
    	if (ad <= 65532){
    	
    		S = ad & 0xFFFF;
    		imm = 1 << 16;
    	}
    	else return 0;
    
    }

    else if (buffer[1][0] == 'r') {
        if (!lecture_et_syntaxe_registre(buffer[1], &S)) {
            return 0;
        }
        S = S & 0x1F;
        imm = 0;
    }
    
    else if (buffer[1][0] == 'h') {
    
    	int i;
		for(i=1; buffer[1][i]; i++){
		
			if(!isxdigit((unsigned char)buffer[1][i])){
				break;
			}
		
		}
		
    	// si on a atteint la fin du for c'est un saut vers une adresse hexadecimale
    	if (buffer[1][i] == '\0') {
    		if (!lecture_et_syntaxe_saut(buffer[1], &S)) {
            	return 0;
       		}
        	S = S & 0xFFFF;
        	imm = 1 << 16;
    	}
    	
    	else {
        	PRINT_ERROR(ETIQ_INVALIDE);
        	return 0;
        }
    }
    
    else { // soit c'est une adresse décimale soit c'est une étiquette non définie
    	
    	int i;
		for(i=0; buffer[1][i]; i++){
		
			if(!isdigit((unsigned char)buffer[1][i])){
				break;
			}
		}
		
		// si on a atteint la fin du for c'est un saut vers une adresse décimale
		if (buffer[1][i] == '\0') {
			if (!lecture_et_syntaxe_saut(buffer[1], &S)) {
            	return 0;
        	}
        	S = S & 0xFFFF;
        	imm = 1 << 16;
		}
        
        // si on arrive ici c'est une étiquette invalide 
        else {
        	PRINT_ERROR(ETIQ_INVALIDE);
        	return 0;
        }
    }

    inst = (code_ope<<27)| imm | S;

    if (size_out<9) {
        PRINT_ERROR(SORTIE_TROP_PETITE);
        return 0;
    }
    sprintf(out, "%08X", inst);
    return 1;
}



static int traduction_ld(char buffer[5][64], int code_ope, int size_out, char *out, int nb_args){ //traduction et détection des erreurs pour les instructions de chargement
    int rn, rd, S, imm, rd_num, rn_res, S_res;
    if (nb_args!=3){
        PRINT_ERROR(NB_ARGS_INVALIDE);
        return 0;
    }

    if(!lecture_et_syntaxe_registre(buffer[1], &rd_num)){
        return 0;
    }
    rd = rd_num<<22;

    if (!lecture_rkS(buffer[2], &rn, &S, &imm)){
        return 0;
    }
    rn_res = (rn&0x1F) << 17;
    if (imm==0){
        S_res = S & 0x1F;
    }
    else {
        S_res = S & 0xFFFF;
    }
    int inst = (code_ope <<27) | rd | rn_res | imm | S_res;
    if(size_out<9) {
        PRINT_ERROR(SORTIE_TROP_PETITE);
        return 0;
    }
    sprintf(out, "%08X", inst);
    return 1;
}

static int traduction_st(char buffer[5][64], int code_ope, int size_out, char *out, int nb_args){//traduction et détection des erreurs pour les instructions de stockage
    int rd, S, imm, rn_num, rn, rd_res, S_res;

    if (nb_args!=3){
        PRINT_ERROR(NB_ARGS_INVALIDE);
        return 0;
    }

    if(!lecture_et_syntaxe_registre(buffer[2], &rn_num)){
        return 0;
    }

    rn = (rn_num & 0x1F)<<17;

    if(!lecture_rkS(buffer[1], &rd, &S, &imm)){
        return 0;
    }

    rd_res = (rd&0x1F) << 22;
        if (imm==0){
            S_res = S & 0x1F;
        }
        else {
            S_res = S & 0xFFFF;
        }

    int inst = (code_ope << 27)| rd_res | rn | imm | S_res;
    if(size_out<9) {
        PRINT_ERROR(SORTIE_TROP_PETITE);
        return 0;
    }
    sprintf(out, "%08X", inst);
    return 1;
}


static int traduction_in_out(char buffer[5][64], int code_ope, int size_out, char *out, int nb_args){//traduction et détection des erreurs pour les instructions in et out
    int rd, inst;

    if (nb_args!=2){
        PRINT_ERROR(NB_ARGS_INVALIDE);
        return 0;
    }

    if(!lecture_et_syntaxe_registre(buffer[1], &rd)){
        return 0;
    }

    inst = (code_ope<<27) | ((rd & 0x1F)<<22);

    if(size_out<9) {
        PRINT_ERROR(SORTIE_TROP_PETITE);
        return 0;
    }
    sprintf(out, "%08X", inst);
    return 1;
}


//fonction PRINCIPALE 
int traduction(const char *ligne, int size_out, char *out){// met dans out la traduction de la ligne donnée en entrée et renvoie 0 si erreur, 1 sinon
    int inst, code_ope, indice_ope;

    for(int p=0; ligne[p]!='\0'; p++){ // pour refuser les multi-lignes
        if(ligne[p]=='\n' && ligne[p+1]!='\0'){
            PRINT_ERROR(SYNTAXE_INVALIDE);
            return 0;
        }
    }

    char buffer[5][64]={{0}}; //initialisation pour éviter d'avoir n'importe quoi dans les cases non utilisées
    int nb_args=cut(ligne, buffer);
    if(nb_args==0) {return 0;} //erreur déjà print dans cut
    indice_ope = indice_operande(buffer[0]);

    if(indice_ope==-1){
        PRINT_ERROR(OP_INCONNUE);
        return 0; 
    }
    code_ope = indice_ope;

    switch(code_ope){
        case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 30 : 
            return traduction_arithmetique(buffer, code_ope, size_out, out, nb_args); 
        case 12: case 13: case 14: 
            return traduction_ld(buffer, code_ope, size_out, out, nb_args); 
        case 15: case 16: case 17: 
            return traduction_st(buffer, code_ope, size_out, out, nb_args);
        case 21: case 22: case 23: case 24: case 25: case 26: case 27: 
            return traduction_jmp(buffer, code_ope, size_out, out, nb_args);
        case 28: case 29: 
            return traduction_in_out(buffer, code_ope, size_out, out, nb_args);
        case 31: 
            inst = (code_ope<<27);
            if (nb_args!=1){
                PRINT_ERROR(NB_ARGS_INVALIDE);
                return 0;
            }
            sprintf(out, "%08X", inst);
            return 1;
        default: 
            PRINT_ERROR(OP_INCONNUE);
            return 0;
    }
}

