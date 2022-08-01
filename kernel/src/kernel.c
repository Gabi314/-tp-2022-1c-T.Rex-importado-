#include "funcionesKernel.h"

int main(void) {

	inicializar_configuracion();
	inicializar_colas();
	inicializar_semaforos();
//	generar_conexiones();



	 pthread_t hilo0;
	 pthread_t hiloAdmin[6];
	 int hiloAdminCreado[6];
	 ejecucionActiva = false;
	 procesoDesalojado = NULL;
	 procesoAFinalizar = NULL;
	 procesoAEjecutar = NULL;


/*
	 	int hiloCreado = pthread_create(&hilo0, NULL,&recibir_consola,socketServidor);
	 	pthread_detach(hiloCreado);

*/
	 	 servidorPrueba = 1;

	 	int hiloCreado = pthread_create(&hilo0, NULL,&recibir_consola_prueba,servidorPrueba);
	 	pthread_detach(hiloCreado);


	 	hiloAdminCreado[0] = pthread_create(&hiloAdmin[0],NULL,&asignar_memoria,NULL);
//	 	hiloAdminCreado[1] = pthread_create(&hiloAdmin[1],NULL,&atender_interrupcion_de_ejecucion,NULL);
	 	hiloAdminCreado[2] = pthread_create(&hiloAdmin[2],NULL,&atenderDesalojo,NULL);
	 	hiloAdminCreado[3] = pthread_create(&hiloAdmin[3],NULL,&readyAExe,NULL);
	// 	hiloAdminCreado[4] = pthread_create(&hiloAdmin[4],NULL,&atenderIO,NULL);
	//	hiloAdminCreado[5] = pthread_create(&hiloAdmin[5],NULL,&desbloquear_suspendido,NULL);



	 	pthread_detach(hiloAdmin[0]);
//	 	pthread_detach(hiloAdmin[1]);
	 	pthread_detach(hiloAdmin[2]);
	 	pthread_detach(hiloAdmin[3]);
	// 	pthread_detach(hiloAdmin[4]);
	//	pthread_detach(hiloAdmin[5]);

		log_info(logger,"termino el while");
	 	while(true){

	 	}


		//conexionConConsola();
		//conexionConCpu();
		//conexionConMemoria();
	}








void inicializar_configuracion(){
	config = config_create("kernel.config");

	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_INFO);
	if(config == NULL) {
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

void generar_conexiones(){
     socketServidor = iniciar_servidor();
     log_info(logger, "Servidor listo para recibir al cliente");
     socketMemoria = crear_conexion(ipMemoria, puertoMemoria);
	 socketCpuDispatch = crear_conexion(ipCpu, puertoCpuDispatch);
	 socketCpuInterrupt = crear_conexion(ipCpu, puertoCpuInterrupt);

}

void inicializar_semaforos(){
	sem_init(&pcbEnNew,0,0);
	sem_init(&pcbEnReady,0,0);
	sem_init(&cpuDisponible,0,1);
	sem_init(&gradoDeMultiprogramacion,0,gradoMultiprogramacionTotal);
	sem_init(&desalojarProceso,0,0);
	sem_init(&procesoEjecutandose,0,0);
	sem_init(&procesoDesalojado,0,1);
	sem_init(&pcbInterrupt,0,0);
	sem_init(&pcbBlocked,0,0);
	sem_init(&pcbExit,0,0);

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


int conexionConMemoria(void){
	socketMemoria = crear_conexion(ipMemoria, puertoMemoria);
	log_info(logger,"Hola memoria, soy Kernel");
	//enviar_mensaje("hola kernel",socketMemoria,MENSAJE);
	enviarPID();

	t_list* listaQueContieneNroTabla1erNivel = list_create();

	int cod_op = recibir_operacion(socketMemoria);

	if(cod_op == PAQUETE){
		listaQueContieneNroTabla1erNivel = recibir_paquete_int(socketMemoria);
	}

	//poner en pcb:
	int nroTabla1erNivel = (int) list_get(listaQueContieneNroTabla1erNivel,0);

	log_info(logger,"Me llego la tabla de primero nivel nro: %d",nroTabla1erNivel);

	return EXIT_SUCCESS;
}



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


