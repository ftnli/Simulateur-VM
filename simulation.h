#ifndef MACHINE_H 
#define MACHINE_H 

#define TAILLE_MEM 65536      // pour la définition de la structure         

typedef struct machine {
    unsigned int  R[32];          
    unsigned int  PC;             
    unsigned char Z, C, N;        
    unsigned char mem[TAILLE_MEM];  
} Machine;


int charger_hexa_txt(Machine *, char *);

int execute_instruction(Machine *);

void machine_zero(Machine *);

#endif