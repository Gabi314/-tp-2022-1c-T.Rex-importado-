#ifndef FUNCIONES_CPU_H_
#define FUNCIONES_CPU_H_

#include "funcionesCpu.h"
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<commons/string.h>
#include<commons/config.h>
#include<string.h>
#include<assert.h>
#include<math.h>
#include <time.h>

#define IP_CPU "127.0.0.1"
#define PUERTO_CPU_DISPATCH 8001 // por ahora este faltan los otros puertos para conectar a kernel

typedef enum
{
	MENSAJE,
	PAQUETE,
	PAQUETE2,
	PAQUETE3,
	PAQUETE4,
	PAQUETE5,
	SUSPENSION,
	DESUSPENSION
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
t_config* config;
t_paquete* paquete;

//--------------  Cpu como servidor de Kernel ---------
int clienteKernel;

int tamanioTotalIdentificadores;
int contadorInstrucciones;
int desplazamiento;
char* tamanioDelProceso;
//Estructuras

typedef struct
{
	int idProceso;
	int tamanioProceso;
	t_list* instrucciones;
	int programCounter;
	int nroTabla1erNivel;
	float estimacionRafaga;
	//int socket_cliente;
} t_pcb;

typedef struct
{
	char* identificador;
	int parametros[2];
} instruccion;


void* recibir_buffer(int*, int);

int iniciar_servidor(void);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);

//Funcion propia de cpu como servidor
void iterator(int value);
int conexionConKernel(void);
void obtenerTamanioIdentificadores(instruccion*);
void agregarInstruccionesAlPaquete(instruccion*);
t_paquete* agregar_a_paquete_kernel_cpu(t_pcb*);
//Funcion propia de cpu como servidor

//--------------  Cpu como cliente de Memoria -------------
int conexionMemoria;
//Variables globales de config
int cantidadEntradasTlb;
char* algoritmoReemplazoTlb;
int retardoDeNOOP;
char* ipMemoria;
int puertoMemoria;
int puertoDeEscuchaDispath;
int puertoDeEscuchaInterrupt;

//Variables direccion logica
int numeroDePagina;
int entradaTabla1erNivel;
int entradaTabla2doNivel;
int desplazamiento;

t_list* tamanioDePagYEntradas;
int tamanioDePagina;
int entradasPorTabla;

//Estructuras
t_list* tlb;

typedef struct
{
	int nroDePagina;
	int nroDeMarco;
	time_t instanteGuardada;
	time_t ultimaReferencia;
} entradaTLB;

int crear_conexion(char* ip, int puertoCpuDispatch);
t_paquete* crear_paquete(int cod_op);

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void* serializar_paquete(t_paquete* paquete, int bytes);
void enviar_mensaje(char* mensaje, int socket_cliente,int cod_op);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

//Funciones propias de Cpu como cliente
t_log* iniciar_logger(void);
t_config* iniciar_config(void);

void enviarEntradaTabla1erNivel();
void enviarEntradaTabla2doNivel();
void enviarDireccionFisica(int,int,int);
void enviarValorAEscribir(uint32_t);
void enviarDireccionesFisicasParaCopia(int,int,int,int);

void terminar_programa(int, t_log*, t_config*);

int conexionConMemoria(void);
void inicializarConfiguraciones();

t_list* inicializarTLB();
void generarListaCamposTLB(t_list*);
void reiniciarTLB();
int chequearMarcoEnTLB(int);
void agregarEntradaATLB(int,int);
void algoritmosDeReemplazoTLB(int,int);
void reemplazarPagina(int,int,int);

void calculosDireccionLogica(int);
void leerTamanioDePaginaYCantidadDeEntradas(t_list*);
int buscarDireccionFisica(int);
int accederAMemoria(int);

//Ciclo de instruccion
instruccion* buscarInstruccionAEjecutar(t_pcb*);
void decode(instruccion*);
void ejecutar(instruccion*);


#endif /*FUNCIONES_CPU_H_*/
