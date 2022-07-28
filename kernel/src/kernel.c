#include "funcionesKernel.h"

t_list* listaInstrucciones;

int main(void) {
	logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	inicializarConfiguraciones();
	//crear_colas();
    //generar_conexiones();

	conexionConConsola();
	cargar_pcb();
	conexionConCpu();
	//conexionConMemoria();
}


void cargar_pcb(){
	pcb = malloc(sizeof(t_pcb));
	pcb -> instrucciones = list_create();

	pcb->instrucciones = listaInstrucciones;
	pcb->idProceso = 0; //esto es por la idea de tomas de hacerlo secuencial, proceso 1 id 0, proceso 2 id 1, etc...
	pcb->program_counter = 0;
	pcb->tamanioProceso = 1024; // viene por consola en realidad
	pcb->tabla_paginas = 0;//viene de memoria
	pcb->estimacion_rafaga = estimacionInicial; // no se si varia


	gradoMultiprogramacionActual = 0; //Arranca en 0 porque no hay procesos en memoria
}

void inicializarConfiguraciones(){
	t_config* config = config_create("kernel.config");
	if(config  == NULL){
		printf("Error leyendo archivo de configuración. \n");
	}

	ipMemoria = config_get_string_value(config, "IP_MEMORIA");
	puertoMemoria = config_get_int_value(config, "PUERTO_MEMORIA");//son intss
	ipCpu = config_get_string_value(config, "IP_CPU");
	puertoCpuDispatch = config_get_int_value(config, "PUERTO_CPU_DISPATCH");//son intss
	puertoCpuInterrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
	puertoKernel = config_get_string_value(config, "PUERTO_ESCUCHA");
	algoritmoPlanificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	estimacionInicial = atoi(config_get_string_value(config, "ESTIMACION_INICIAL"));
	alfa = atoi(config_get_string_value(config, "ALFA"));
	gradoDeMultiprogramacion = atoi(config_get_string_value(config, "GRADO_MULTIPROGRAMACION"));
	tiempoMaximoBloqueado = config_get_string_value(config, "TIEMPO_MAXIMO_BLOQUEADO");
}

void crear_colas(){
	colaNew = queue_create();
	colaReady = queue_create(); //deberia ser una fila, pq la vamos a tener q ordenar con el algoritmo srt
	colaSuspendedReady = queue_create();
	colaExe = queue_create();
	colaBlocked = queue_create();
	colaSuspendedBlocked = queue_create();
	colaExit = queue_create();
}

void generar_conexiones(){
     //socketServidor = iniciar_servidor();
     //log_info(logger, "Servidor listo para recibir al cliente");
     socketMemoria = crear_conexion(ipMemoria, puertoMemoria);//estan pasando un string en los puertos
     //socketCpuDispatch = crear_conexion(ipCpu, puertoCpuDispatch);
	 //falta también las conexiones con cpu para interrupciones
}


//---------------------------------------------------------------------------------------------

int conexionConMemoria(void){
	socketMemoria = crear_conexion(ipMemoria, puertoMemoria);
	log_info(logger,"Hola memoria, soy Kernel");
	//enviar_mensaje("hola kernel",socketMemoria,MENSAJE);
	enviarPID();

	t_list* listaQueContieneNroTabla1erNivel = list_create();

	int cod_op = recibir_operacion(socketMemoria);

	if(cod_op == NRO_TP1){
		listaQueContieneNroTabla1erNivel = recibir_paquete_int(socketMemoria);
	}

	//poner en pcb:
	int nroTabla1erNivel = (int) list_get(listaQueContieneNroTabla1erNivel,0);

	log_info(logger,"Me llego la tabla de primero nivel nro: %d",nroTabla1erNivel);

	return EXIT_SUCCESS;
}

void enviarPID(){//pasar entrada y nroTabla1ernivel
	t_paquete* paquete = crear_paquete(NRO_TP1);
	// Leemos y esta vez agregamos las lineas al paquete
	agregar_a_paquete(paquete,&pidKernel,sizeof(pidKernel));


	log_info(logger,"Le envio a memoria mi pid que es %d",pidKernel);
	enviar_paquete(paquete,socketMemoria);
	eliminar_paquete(paquete);
}




//---------------------------------------------------------------------------------------------

//preguntar por el switch
int conexionConConsola(void){
	logger = log_create("./kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);

	int server_fd = iniciar_servidor();
	log_info(logger, "Kernel listo para recibir a consola");
	int cliente_fd = esperar_cliente(server_fd);

	while (1) {
		int cod_op = recibir_operacion(cliente_fd);
		switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(cliente_fd);
			break;
			case RECIBIR_INSTRUCCIONES:
				listaInstrucciones = list_create();
				listaInstrucciones = recibir_paquete(cliente_fd);
				
				//list_iterate(instrucciones, (void*) iterator);
			break;
			case -1:
				log_error(logger, "La consola se desconecto. Finalizando Kernel");
			return EXIT_FAILURE;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
			}
	  return EXIT_SUCCESS;
}

// -----------------------------------------------------------------------------------------------------


int conexionConCpu(void){

    socketCpuDispatch = crear_conexion(ipCpu, puertoCpuDispatch);
	
	log_info(logger,"Hola cpu, soy Kernel");

	agregar_a_paquete_kernel_cpu(pcb); // aca ya se envia

	
	
	return EXIT_SUCCESS;
}



void terminar_programa(int conexion, t_log* logger, t_config* config){
	/* Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config)
	  con las funciones de las commons y del TP mencionadas en el enunciado */
	log_destroy(logger);
	config_destroy(config);

	liberar_conexion(conexion);
}

void iterator(int value) {
	log_info(logger,"%d",value);
}


