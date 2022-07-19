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
	PAQUETE2,
	PAQUETE3,
	PAQUETE4,
	PAQUETE5
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
int clienteCpu;

void* recibir_buffer(int*, int);
void recibir_mensaje(int);

//Para recibir de kernel y cpu
int iniciar_servidor(void);
int esperar_cliente(int);

t_list* recibir_pedido_deTamPagYCantEntradas(int);
t_list* recibir_paquete_int(int);

void recibir_mensaje(int);
int recibir_operacion(int);
//---------------------------

//Para envios a cpu
void enviarTamanioDePaginaYCantidadDeEntradas(int socket_cliente);
void enviarNroTabla2doNivel(int,int);
void enviarMarco(int, int);
void enviar_mensaje(char*,int);

t_paquete* crear_paquete(int);

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

void* memoria; // espacio de usuario de la memoria

int nroTablaDePaginas1erNivel;
int entradaTablaDePaginas1erNivel;
t_list* nroTabla1erNivelYentrada;

//Tabla de paginas
typedef struct{
	int numeroDeEntradaPorProceso;
	int numeroMarco;
	int presencia;
	int uso;
	int modificado;

}entradaTabla2doNivel;

typedef struct{
	int numeroDeMarco;//Hacer lista de marcos, el indice en la lista es el numero de marco, la cantidad de marcos es
	uint32_t valor;//no haria falta
	int marcoLibre;// tam memoria/tam pagina
}marco;

typedef struct{// capaz usar diccionario
	t_list* entradas;
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
marco* buscarMarco(int);

void crearSwap(int);

void escribirElPedido(uint32_t,int,int);
uint32_t leerElPedido(int,int);
void copiar(int,int,int,int);

void cargarEntradasDeTabla2doNivel(t_segundoNivel*);// con esta funcion que antes se llamaba cargar paginas
//void cargarPaginas(t_segundoNivel*);
int leerYRetornarNroTabla2doNivel(t_list*);
void modificarPaginaACargar(entradaTabla2doNivel*, int); //Antes estaba pagina en entrada

marco* siguienteMarcoLibre();
void sacarMarcoAPagina(entradaTabla2doNivel*);//Antes estaba pagina en entrada
int marcoSegunIndice(int,int);
int algoritmoClock(t_list*);
int algoritmoClockM(t_list*);
int indiceDeEntradaAReemplazar(int);

char* nombreArchivoProceso(int);
void escribirEnSwap(int,int,int,int);



#endif /* FUNCIONES_MEMORIA_H_*/

