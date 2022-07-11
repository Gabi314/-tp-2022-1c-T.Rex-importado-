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
	PAQUETE,
	PAQUETE2
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

t_log* logger;

void* recibir_buffer(int*, int);
void recibir_mensaje(int);

//Para recibir de kernel y cpu
int iniciar_servidor(void);
int esperar_cliente(int);

t_list* recibir_pedido_deTamPagYCantEntradas(int);
t_list* recibir_nroTabla1erNivel_entradaTabla1erNivel(int);

void recibir_mensaje(int);
int recibir_operacion(int);
//---------------------------

//Para enviar a cpu(tam_paginas y cant_entradas) y a kernel(nro de tabla de pags de 1er nivel)
void enviarTamanioDePaginaYCantidadDeEntradas(int socket_cliente);
void enviarNroTabla2doNivel(int,int);

t_paquete* crear_paquete(void);
t_paquete* crear_otro_paquete(void);

void agregar_a_paquete(t_paquete* paquete, void* valor,int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

//-----------------------------

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

int nroTablaDePaginas1erNivel;
int entradaTablaDePaginas1erNivel;
t_list* nroTabla1erNivelYentrada;

//Tabla de paginas
typedef struct{
	int numeroDePagina;//depnde en que linea ponga esta variable cambia su valor(basura, memory leak)
	int numeroMarco;
	int presencia;
	int uso;
	int modificado;

}pagina;

typedef struct{
	int numeroDeMarco;//Hacer lista de marcos, el indice en la lista es el numero de marco, la cantidad de marcos es
	uint32_t valor;
	int marcoLibre;// tam memoria/tam pagina
}marco;

typedef struct{// capaz usar diccionario
	t_list* paginas;
	int numeroTabla;
}t_segundoNivel;

typedef struct{// capaz usar diccionario
	t_list* tablasDeSegundoNivel; // int tablasDeSegundoNivel[entradas]
	int pid;
}t_primerNivel;




void crearConfiguraciones();
void inicializarEstructuras();
void inicializarMarcos();
int numeroTabla2doNivelSegunIndice(int,int);
int buscarNroTablaDe1erNivel(int);
int buscarNroTablaDe2doNivel(int);
void escribirEnSwap(int);
void cargarPaginas(t_segundoNivel*);
int leerYRetornarNroTabla2doNivel(t_list*);
void modificarPaginaACargar(pagina*, int);
int siguienteMarcoLibre();
void sacarMarcoAPagina(pagina*);
int algoritmoClock(t_list*);
int algoritmoClockM(t_list*);



#endif /* FUNCIONES_MEMORIA_H_*/

