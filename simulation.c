#include <stdio.h>
#include <stdlib.h>

/* Fichier d'entête avec prototypes structure et macro globale */
#include "simulation.h" 

/*Extension de signe */
static int ext_signe16(unsigned short x) {
    // force l'interprétation de x comme short signé (16 bits),
    // puis l'étend en int (32 bits)
    return (int)( (short)x );
}

/* Lire et écrire en mémoire */
static unsigned char lire_mem8(Machine *m, unsigned int a) {
    return (*m).mem[a & 0xFFFF];
    /*Le masque force l’adresse a à rester dans [0;65535](64 Ko) car 0xFFFF = 65535*/
}

static unsigned short lire_mem16(Machine *m, unsigned int a) {
    a &= 0xFFFF;
    unsigned short b0 = (unsigned short)(*m).mem[a];
    unsigned short b1 = (unsigned short)(*m).mem[(a + 1) & 0xFFFF];
    return (unsigned short)(b0 | (unsigned short)(b1 << 8));
}

static unsigned int lire_mem32(Machine *m, unsigned int a) {
    a &= 0xFFFF;
    unsigned int b0 = (unsigned int)(*m).mem[a];
    unsigned int b1 = (unsigned int)(*m).mem[(a + 1) & 0xFFFF];
    unsigned int b2 = (unsigned int)(*m).mem[(a + 2) & 0xFFFF];
    unsigned int b3 = (unsigned int)(*m).mem[(a + 3) & 0xFFFF];
    return (b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
}

static void ecrire_mem8(Machine *m, unsigned int a, unsigned char v) {
    (*m).mem[a & 0xFFFF] = v;
}

static void ecrire_mem16(Machine *m, unsigned int a, unsigned short v) {
    a &= 0xFFFF;
    (*m).mem[a] = (unsigned char)(v & 0xFF);
    (*m).mem[(a + 1) & 0xFFFF] = (unsigned char)((v >> 8) & 0xFF);
}

static void ecrire_mem32(Machine *m, unsigned int a, unsigned int v) {
    a &= 0xFFFF;
    (*m).mem[a] = (unsigned char)(v & 0xFFu);
    (*m).mem[(a + 1) & 0xFFFF] = (unsigned char)((v >> 8) & 0xFF);
    (*m).mem[(a + 2) & 0xFFFF] = (unsigned char)((v >> 16) & 0xFF);
    (*m).mem[(a + 3) & 0xFFFF] = (unsigned char)((v >> 24) & 0xFF);
}

/* Mise à jour des flags*/

static void set_ZN(Machine *m, unsigned int v) {
    (*m).Z = (unsigned char)(v == 0);
    (*m).N = (unsigned char)((v >> 31) & 1);
}

/* Fonction decode : extraction des champs binaires de l'instruction machine
   code_op: bits 27 à 31
   rd    : bits 22 à 26
   rn    : bits 17 à 21
   Imm   : bit 16
   src2  : bits 0 à 15 */
   
static void decode(unsigned int inst,int *code_op, int *rd, int *rn, int *Imm, unsigned short *src2){
    *code_op = (int)((inst >> 27) & 0x1F);
    *rd     = (int)((inst >> 22) & 0x1F);
    *rn     = (int)((inst >> 17) & 0x1F);
    *Imm    = (int)((inst >> 16) & 0x01);
    *src2   = (unsigned short)(inst & 0xFFFF);
}

/* Charger hexa.txt dans le tableau mémoire 
(8 caractères hexadécimaux par ligne = 1 instruction 32 bits)*/
int charger_hexa_txt(Machine *m, char *c) {
    FILE *f = fopen(c, "r");
    if (!f) { 
    	fprintf(stderr, "Erreur : fichier %s introuvable\n", c); 
    	return 0; 
    }
	char ligne[256];
    unsigned int i = 0;
    while (fgets(ligne, sizeof(ligne), f)) { 
    	char *p=ligne;
    	char hex8[9] = {0};
    	int k = 0;
        while (k < 8) {
        	hex8[k++] = *p;
            p++;
        }
/* strtoul convertit la chaîne de caractères hexadécimale en unsigned long 
(on convertit ensuite en unsigned int 32 bits)*/  
        unsigned long v = strtoul(hex8, NULL, 16);
        unsigned int inst = (unsigned int)v;
        ecrire_mem32(m, i * 4, inst);
        i++;
    }
    fclose(f);
    return 1;
}

/* Execution instruction par instruction */
int execute_instruction(Machine *m) {
    /*Lecture mémoire*/
    unsigned int inst = lire_mem32(m, (*m).PC);

    /* Décoder */
    int code_op, rd, rn, Imm;
    unsigned short src2;
    decode(inst, &code_op, &rd, &rn, &Imm, &src2);

    /* Construire S :
       - Imm=0 : rm = bits 0 à 4 de src2, S = R[rm]
       - Imm=1 : S = ext_signe16(src2) (immédiat signé 16 bits étendu sur 32 bits)
    */
    unsigned int rm = (unsigned int)(src2 & 0x001F);
    int S_signed = (Imm ? ext_signe16(src2) : (int) (*m).R[rm]);
    unsigned int S_u = (unsigned int)S_signed;

    /* PC par défaut : instruction suivante */
    unsigned int nextPC = (*m).PC + 4;

    switch (code_op) {

        /* arithmétique/logique: 0 à 7*/
        case 0: { /* or */
            unsigned int res = (*m).R[rn] | (unsigned int)S_signed;
            if (rd != 0) (*m).R[rd] = res;
            (*m).C = 0;
            set_ZN(m, res);
        } break;

        case 1: { /* and */
            unsigned int res = (*m).R[rn] & (unsigned int)S_signed;
            if (rd != 0) (*m).R[rd] = res;
            (*m).C = 0;
            set_ZN(m, res);
        } break;

        case 2: { /* xor */
            unsigned int res = (*m).R[rn] ^ (unsigned int)S_signed;
            if (rd != 0) (*m).R[rd] = res;
            (*m).C = 0;
            set_ZN(m, res);
        } break;

        case 3: { /* add */
            /* somme sur 64 bits pour carry */
            unsigned long long sum = (unsigned long long)(*m).R[rn] + 
            						(unsigned long long)(unsigned int)S_signed;

            unsigned int res = (unsigned int)sum;
            if (rd != 0) (*m).R[rd] = res;

            /* carry = bit 32 si on compte à partir de 0 */
            (*m).C = (unsigned char)((sum >> 32) & 1);
            set_ZN(m, res);
        } break;

      case 4: { /* sub */
   			unsigned int a = (*m).R[rn];
   			unsigned int b = (unsigned int)S_signed;
   			
    		/* a - b = a + (~b) + 1, calcul sur 64 bits */
    		unsigned long long tmp = (unsigned long long)a + (unsigned long long)(~b) + 1;
   			unsigned int res = (unsigned int)tmp;
    		if (rd != 0)
        	(*m).R[rd] = res;
    		/* C = 1 s’il y a une retenue */
    		(*m).C = (unsigned char)((tmp >> 32) & 1);
    		set_ZN(m, res);
		} break;


        case 5: { /* mul : garde 16 bits faibles de S et rn */
            short a16 = (short)((*m).R[rn] & 0xFFFFu);   // 16 bits faibles interprétés signés
            short b16 = (short)(S_u & 0xFFFFu);          // idem pour S
            int prod  = (int)a16 * (int)b16;             // multiplication signée
            if (rd != 0) {(*m).R[rd] = (unsigned int)prod; }            // stockage des bits sur 32 bits
            (*m).C = 0;
            set_ZN(m, (unsigned int)prod);
        } break;

        case 6: { /* div */
            int denom = S_signed;
            int num =(int)(*m).R[rn];
            unsigned int res;
            if (denom == 0) {
                printf("Erreur : division par 0 à PC=0x%04X\n", (*m).PC);
                return 0;         //division par 0 donne une erreur et stoppe la simulation 
            } else {
                int q = num / denom; //division signée
                res= (unsigned int) q;
                if (rd != 0) (*m).R[rd] = res;
                (*m).C = 0;
                set_ZN(m, res);
            }
        } break;

       case 7: { /* shr */
    		int sh = S_signed;                 
    		unsigned int val = (*m).R[rn];
    		unsigned int res;
    		if (sh == 0) {
        		res = val;
       			(*m).C = 0;
    		}
    		else if (sh > 0) {
        /* décalage à DROITE*/
        		if (sh >= 32) {
            		(*m).C = (unsigned char)((val >> 31) & 1);
            		res = 0;
        		} else {
           			(*m).C = (unsigned char)((val >> (sh - 1)) & 1);
            		res = val >> sh;
        		}
    		}
    		else { /* sh < 0 */
        /* décalage à GAUCHE */
        		int lsh = -sh;
        		if (lsh >= 32) {
            		(*m).C = (unsigned char)(val & 1); 
            		res = 0;
        		} else {
            		(*m).C = (unsigned char)((val >> (32 - lsh)) & 1);
            		res = val << lsh;
        		}
   		 	}	
   			if (rd != 0) (*m).R[rd] = res;
   			set_ZN(m, res);
		} break;


        /*  transferts: 12 à 17  */
        case 12: /* ldb rd, (rn)S */
        case 13: /* ldh rd, (rn)S */
        case 14: { /* ldw rd, (rn)S */
            unsigned int addr = ((*m).R[rn] + (unsigned int)S_signed) & 0xFFFF;
            unsigned int res;
            if (code_op == 12) {
                /* lecture de 8 bits, puis extension de signe */
                char b = (char)lire_mem8(m, addr);
                res = (unsigned int)(int)b;    
            } 
            else if (code_op == 13) {
                /* lecture de 16 bits, puis extension de signe */
                short h = (short)lire_mem16(m, addr);
                res = (unsigned int)(int)h;
            } 
            else {
            /*lecture de 32 bits*/
                res = lire_mem32(m, addr);
            }
            if (rd != 0) (*m).R[rd] = res;
            (*m).C = 0;
            set_ZN(m, res);
        } break;

        case 15: /* stb (rd)S, rn */
        case 16: /* sth (rd)S, rn */
        case 17: { /* stw (rd)S, rn */
            unsigned int addr = ((*m).R[rd] + (unsigned int)S_signed) & 0xFFFF;
            unsigned int v = (*m).R[rn];
            if (code_op == 15) {
                ecrire_mem8(m, addr, (unsigned char)(v & 0xFF));
            } else if (code_op == 16) {
                ecrire_mem16(m, addr, (unsigned short)(v & 0xFFFF));
            } else {
                ecrire_mem32(m, addr, v);
            }
            (*m).C = 0;
            set_ZN(m, v);
        } break;

        /* sauts: 21 à 27 */
        case 21: { /* jmp S */
            nextPC = S_u & 0xFFFF;
        } break;

        case 22: { /* jzs S */
            if ((*m).Z) nextPC = S_u & 0xFFFF;
        } break;

        case 23: { /* jzc S */
            if (!(*m).Z) nextPC = S_u & 0xFFFF;
        } break;

        case 24: { /* jcs S */
            if ((*m).C) nextPC = S_u & 0xFFFF;
        } break;

        case 25: { /* jcc S */
            if (!(*m).C) nextPC = S_u & 0xFFFF;
        } break;

        case 26: { /* jns S (N set -> négatif) */
            if ((*m).N) nextPC = S_u & 0xFFFF;
        } break;

        case 27: { /* jnc S (N clear -> pas négatif) */
            if (!(*m).N) nextPC = S_u & 0xFFFF;
        } break;

        /* in/out: 28 et 29*/
        case 28: { /* in rd */
        	int x;
            printf("[IN ] Entrez un entier: \n");
            scanf("%d", &x);
            if (rd != 0) (*m).R[rd] = (unsigned int)x;
            (*m).C = 0;
            set_ZN(m, (unsigned int)x);
        } break;

        case 29: { /* out rd */
            printf("[OUT] %d\n", (int)(*m).R[rd]); /* affichage signé */
            (*m).C = 0;
            set_ZN(m, (*m).R[rd]);
        } break;

        /*divers: 30 et 31 */
        case 30: { /* rnd rd, rn, S : [a, b[ */
            int a = (int)(*m).R[rn];
            int b = S_signed;
            unsigned int res;
            if (b <= a) {
                res = (unsigned int)a;
            } else {
                int range = b - a;              /* range > 0 ici */
                res = (unsigned int)(a + (rand() % range));
            }
            if (rd != 0) (*m).R[rd] = res;
            (*m).C = 0;
            set_ZN(m, res);
        } break;

        case 31: { /* hlt */
            return 0;
        }

        default:
            fprintf(stderr, "Code opératoire inconnu %d a PC=%u (0x%04X)\n",
                    code_op, (*m).PC, (unsigned)(*m).PC);
            return 0;
    }

    /* r0 toujours à 0 */
    
    (*m).R[0] = 0;

    /* mise à jour PC */
    
    (*m).PC = nextPC & 0xFFFF;

    return 1;
}

void machine_zero(Machine *m) {
    int i;
    /* Zéro registres */
    for (i = 0; i < 32; i++) {
        (*m).R[i] = 0;
    }
    /* Zéro PC et flags */
    (*m).PC = 0;
    (*m).Z = 0;
    (*m).C = 0;
    (*m).N = 0;
    /* Zéro mémoire */
    for (i = 0; i < (int)TAILLE_MEM; i++) {
        (*m).mem[i] = (unsigned char)0;
    }
}

