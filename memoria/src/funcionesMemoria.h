#ifndef FUNCIONES_MEMORIA_H_
#define FUNCIONES_MEMORIA_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<string.h>
#include<commons/string.h>
#include<assert.h>
#include<stdbool.h>

#define IP_MEMORIA "127.0.0.1"
#define PUERTO_MEMORIA 8002

typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

t_log* logger;

void* recibir_buffer(int*, int);

int iniciar_servidor(void);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);

//Funcion propia de Memoria como servidor
void iterator(char* value);
int conexionConCpu(void);

//Variables globales de config
int tamanioDeMemoria;
int tamanioDePagina;
int entradasPorTabla;
int retardoMemoria; //Tiempo en milisegundos que se deberá esperar para dar una respuesta al CPU
char* algoritmoDeReemplazo;
int marcosPorProceso;
int retardoSwap; //Tiempo en milisegundos que se deberá esperar para cada operación del SWAP (leer/escribir)

//Tabla de paginas
typedef struct{
	int numeroDePagina;//depnde en que linea ponga esta variable cambia su valor(basura, memory leak)
	int numeroMarco;
	int presencia;
	int uso;
	int modificado;

}pagina;

typedef struct{// capaz usar diccionario
	t_list* paginas;
}t_segundoNivel;

typedef struct{// capaz usar diccionario
	t_list* tablasDeSegundoNivel; // int tablasDeSegundoNivel[entradas]
	int pid;
}t_primerNivel;



void crearConfiguraciones();
void inicializarEstructuras();
int buscarNroPaginaDe1erNivel(int);
void escribirEnSwap(int);

#endif /* FUNCIONES_MEMORIA_H_*/

