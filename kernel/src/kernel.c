#include "funcionesKernel.h"



int main(void) {

	inicializar_configuracion();
	inicializar_colas();
	inicializar_semaforos();
	generar_conexiones();


	 pthread_t hilo0, hiloAdmin[6];
	 int hiloAdminCreado[6];


	 	/*

	 	int hiloCreado = pthread_create(&hilo0, NULL,&recibir_consola,socketServidor);
	 	pthread_detach(hiloCreado);
		*/

	 	 servidorPrueba = 1;

	 	int hiloCreado = pthread_create(&hilo0, NULL,&recibir_consola_prueba,servidorPrueba);
	 	pthread_detach(hiloCreado);


	 	hiloAdminCreado[0] = pthread_create(&hiloAdmin[0],NULL,&asignar_memoria,NULL);
	 	hiloAdminCreado[1] = pthread_create(&hiloAdmin[1],NULL,&finalizar_proceso_y_avisar_a_memoria,NULL);
	 	hiloAdminCreado[2] = pthread_create(&hiloAdmin[2],NULL,&finalizar_proceso_y_avisar_a_consola,NULL);
	 	hiloAdminCreado[3] = pthread_create(&hiloAdmin[3],NULL,&readyAExe,NULL);
	 	hiloAdminCreado[4] = pthread_create(&hiloAdmin[4],NULL,&suspender,NULL);
		hiloAdminCreado[5] = pthread_create(&hiloAdmin[5],NULL,&desbloquear_suspendido,NULL);


// TRANSICIONES QUE FALTAN EN HILOS
// exec a blocked
// blocked a ready
// readySuspended a ready

	 	pthread_detach(hiloAdmin[0]);
	 	pthread_detach(hiloAdmin[1]);
	 	pthread_detach(hiloAdmin[2]);
	 	pthread_detach(hiloAdmin[3]);
		pthread_detach(hiloAdmin[4]);
		pthread_detach(hiloAdmin[5]);

	 	while(1);
		//conexionConConsola();
		//conexionConCpu();
		//conexionConMemoria();
	}








void inicializar_configuracion(){
	t_config* config;

	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_INFO);
	if((config = config_create("kernel.config")) == NULL){
			printf("Error leyendo archivo de configuración. \n");
		}
	//ipKernel = config_get_string_value(config, "IP");
	pidKernel = 0; // ID del kernel
	ipMemoria = config_get_string_value(config, "IP_MEMORIA");
	puertoMemoria = config_get_string_value(config, "PUERTO_MEMORIA");
	ipCpu = config_get_string_value(config, "IP_CPU");
	puertoCpuDispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
	puertoCpuInterrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
	puertoKernel = config_get_string_value(config, "PUERTO_ESCUCHA");
	algoritmoPlanificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	estimacionInicial = atoi(config_get_string_value(config, "ESTIMACION_INICIAL"));
	alfa = atoi(config_get_string_value(config, "ALFA"));
	gradoMultiprogramacionTotal = atoi(config_get_string_value(config, "GRADO_MULTIPROGRAMACION"));
	tiempoMaximoBloqueado = atoi(config_get_string_value(config, "TIEMPO_MAXIMO_BLOQUEADO"));
	gradoMultiprogramacionActual = 0; //Arranca en 0 porque no hay procesos en memoria
}


void inicializar_colas(){
	colaNew = queue_create();
	colaReady = list_create();
	colaSuspendedReady = queue_create();
	colaBlocked = list_create();
	colaSuspendedBlocked = list_create();
	colaExit = list_create();
}

void generar_conexiones(){
     socketServidor = iniciar_servidor();
     log_info(logger, "Servidor listo para recibir al cliente");
     socketMemoria = crear_conexion(ipMemoria, puertoMemoria);
	 socketCpuDispatch = crear_conexion(ipCpu, puertoCpuDispatch);
	 socketCpuInterrupt = crear_conexion(ipCpu, puertoCpuInterrupt);
	 //falta también las conexiones con cpu para interrupciones
}

void inicializar_semaforos(){
	sem_init(&pcbEnNew,0,0);
	sem_init(&pcbEnReady,0,0);
	sem_init(&cpuDisponible,0,1);
	sem_init(&gradoDeMultiprogramacion,0,gradoMultiprogramacionTotal);
	pthread_mutex_init(&asignarMemoria,NULL);
	pthread_mutex_init(&colaReadyFIFO,NULL);
	pthread_mutex_init(&colaReadySRT,NULL);
	pthread_mutex_init(&ejecucion,NULL);


}

t_log* iniciar_logger(void){
	return log_create("./kernel.log","KERNEL",true,LOG_LEVEL_INFO);
}

void terminar_programa(int conexion, t_log* logger, t_config* config){
	/* Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config)
	  con las funciones de las commons y del TP mencionadas en el enunciado */
	log_destroy(logger);
	config_destroy(config);

	liberar_conexion(conexion);
}

void iterator(char* value) {
	log_info(logger,"%s", value);
}

//---------------------------------------------------------------------------------------------

/*void conexionConMemoria(void){
	int conexion;
	char* ip;
	int puerto;

	t_log* logger = iniciar_logger();
	t_config* config = iniciar_config();

	ip = config_get_string_value(config,"IP_MEMORIA");
	puerto =  config_get_int_value(config,"PUERTO_MEMORIA");

	// Creamos una conexión hacia el servidor
	conexion = crear_conexion(ip, puerto);

    // Armamos y enviamos el paquete
	paquete(conexion, "pido archivo configuracion de memoria");

	terminar_programa(conexion, logger, config);
}*/



//---------------------------------------------------------------------------------------------

//preguntar por el switch
/*int conexionConConsola(void){
	logger = log_create("./kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);

	int server_fd = iniciar_servidor();
	log_info(logger, "Kernel listo para recibir a consola");
	int cliente_fd = esperar_cliente(server_fd);

	t_list* instrucciones;

	while (1) {
		int cod_op = recibir_operacion(cliente_fd);
		switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(cliente_fd);
			break;
			case PAQUETE:
				instrucciones = recibir_paquete(cliente_fd);

				log_info(logger, "Me llegaron las siguientes instrucciones: \n");
				list_iterate(instrucciones, (void*) iterator);
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
*/
// -----------------------------------------------------------------------------------------------------


/*void conexionConCpu(void){

	int conexion;
	char* ip;
	int puerto;
	char* pcb; //deberia ser una estructura

	t_log* logger = iniciar_logger();
	t_config* config = iniciar_config();

	ip = config_get_string_value(config,"IP_CPU");
	puerto =  config_get_int_value(config,"PUERTO_CPU_DISPATCH");
	pcb = "Aca le mandaria el pcb";

	// Creamos una conexión hacia el servidor
    conexion = crear_conexion(ip, puerto);

	// Armamos y enviamos el paquete
	paquete(conexion,pcb);

	terminar_programa(conexion, logger, config);
}*/



/*t_config* iniciar_config(void){
	return config_create("kernel.config");
}*/

/*void paquete(int conexion, char* mensaje){

	t_paquete* paquete = crear_paquete();

	// Leemos y esta vez agregamos las lineas al paquete
    agregar_a_paquete(paquete,mensaje,strlen(mensaje) + 1);
    //free(pcb); tira terrible error

    enviar_paquete(paquete,conexion);
    eliminar_paquete(paquete);
}*/
//Revisar ubicacion


