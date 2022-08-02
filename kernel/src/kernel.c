#include "funcionesKernel.h"


int main(int argc, char *argv[]) {
	logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);

	if(argc < 2) {
	    log_error(logger,"Falta un parametro");
	    return EXIT_FAILURE;
	}


	inicializarConfiguraciones(argv[1]);


	inicializar_colas();
	inicializar_semaforos();


	pthread_t hilo0;
	pthread_t hiloAdmin[6];
	int hiloAdminCreado[6];

	ejecucionActiva = false;
	procesoDesalojado = NULL;

	socketServidor = iniciar_servidor();

	int hiloCreado = pthread_create(&hilo0, NULL,&recibir_consola,socketServidor);
	pthread_detach(hiloCreado);



	hiloAdminCreado[0] = pthread_create(&hiloAdmin[0],NULL,&asignar_memoria,NULL);
	//hiloAdminCreado[1] = pthread_create(&hiloAdmin[1],NULL,&atender_interrupcion_de_ejecucion,NULL); // problemas con esto
	hiloAdminCreado[2] = pthread_create(&hiloAdmin[2],NULL,&atenderDesalojo,NULL);
	hiloAdminCreado[3] = pthread_create(&hiloAdmin[3],NULL,&readyAExe,NULL);
	hiloAdminCreado[4] = pthread_create(&hiloAdmin[4],NULL,&atenderIO,NULL);
	hiloAdminCreado[5] = pthread_create(&hiloAdmin[5],NULL,&desbloquear_suspendido,NULL);



	pthread_detach(hiloAdmin[0]);
	//pthread_detach(hiloAdmin[1]);
	pthread_detach(hiloAdmin[2]);
	pthread_detach(hiloAdmin[3]);
	pthread_detach(hiloAdmin[4]);
	pthread_detach(hiloAdmin[5]);

	log_info(logger,"termino el while");

	sem_wait(&kernelSinFinalizar);

	//int nroTabla1erNivel = conexionConMemoria();
	//conexionConConsola();

	//cargar_pcb(nroTabla1erNivel);
	//conexionConCpu();

}


//void cargar_pcb(int nroTabla1erNivel){
//	pcb = malloc(sizeof(t_pcb));
//	pcb -> instrucciones = list_create();
//
//	pcb->instrucciones = listaInstrucciones;
//	pcb->idProceso = 0; //esto es por la idea de tomas de hacerlo secuencial, proceso 1 id 0, proceso 2 id 1, etc...
//	pcb->program_counter = 0;
//	pcb->tamanioProceso = tamanioDelProceso;
//	pcb->tabla_paginas = nroTabla1erNivel;
//	pcb->estimacion_rafaga = estimacionInicial; // no se si varia
//
//}

void inicializarConfiguraciones(char* unaConfig){
	t_config* config = config_create(unaConfig);
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
	gradoMultiprogramacionTotal = atoi(config_get_string_value(config, "GRADO_MULTIPROGRAMACION"));
	tiempoMaximoBloqueado = atoi(config_get_string_value(config, "TIEMPO_MAXIMO_BLOQUEADO"));
}


void inicializar_colas(){
	colaNew = queue_create();
	colaReady = list_create();
	colaSuspendedReady = queue_create();
	colaBlocked = list_create();
	colaSuspendedBlocked = list_create();
	colaExit = list_create();
	listaDeConsolas = list_create();
}

void inicializar_semaforos(){
	sem_init(&pcbEnNew,0,0);
	sem_init(&pcbEnReady,0,0);
	sem_init(&cpuDisponible,0,1);
	sem_init(&gradoDeMultiprogramacion,0,gradoMultiprogramacionTotal);
	sem_init(&desalojarProceso,0,0);
	sem_init(&procesoEjecutandose,0,0);
	sem_init(&procesoDesalojadoSem,0,1);
	sem_init(&pcbInterrupt,0,0);
	sem_init(&pcbBlocked,0,0);
	sem_init(&pcbExit,0,0);
	sem_init(&kernelSinFinalizar,0,0);

	pthread_mutex_init(&asignarMemoria,NULL);
	pthread_mutex_init(&obtenerProceso,NULL);
	pthread_mutex_init(&ejecucion,NULL);
	pthread_mutex_init(&procesoExit,NULL);
	pthread_mutex_init(&consolasExit,NULL);
	pthread_mutex_init(&desalojandoProceso,NULL);
	pthread_mutex_init(&consolaNueva,NULL);
	pthread_mutex_init(&encolandoPcb,NULL);
	pthread_mutex_init(&mutexExit,NULL);
	pthread_mutex_init(&mutexInterrupt,NULL);
	pthread_mutex_init(&mutexIO,NULL);
	pthread_mutex_init(&bloqueandoProceso,NULL);
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

	int nroTabla1erNivel = (int) list_get(listaQueContieneNroTabla1erNivel,0);

	log_info(logger,"Me llego la tabla de primero nivel nro: %d",nroTabla1erNivel);

	return nroTabla1erNivel;
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
int conexionConConsola(int cliente_fd){
	logger = log_create("./kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);

	//int server_fd = iniciar_servidor();
	log_info(logger, "Kernel listo para recibir a consola");
	//int cliente_fd = esperar_cliente(socketServidor);

	t_list* listaQueContieneTamProceso;

	while (1) {
		int cod_op = recibir_operacion(cliente_fd);
		switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(cliente_fd);
			break;
			case RECIBIR_INSTRUCCIONES:
				listaInstrucciones = list_create();
				listaInstrucciones = recibir_paquete(cliente_fd);
				log_info(logger,"me llegaron las instrucciones");
				//list_iterate(instrucciones, (void*) iterator);
			break;
			case RECIBIR_TAMANIO_DEL_PROCESO:
				listaQueContieneTamProceso = list_create();
				listaQueContieneTamProceso = recibir_paqueteInt(cliente_fd);

				tamanioDelProceso = (int) list_get(listaQueContieneTamProceso,0);
				log_info(logger,"Me llego el tamanio de proceso %d",tamanioDelProceso);
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


int conexionConCpu(t_pcb* pcb){

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


