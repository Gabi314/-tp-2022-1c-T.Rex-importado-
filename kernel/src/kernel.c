#include "funcionesKernel.h"

int main(void) {

	cargar_pcb();
	crear_colas();
    generar_conexiones();

	//conexionConConsola();
	//conexionConCpu();
	//conexionConMemoria();
}


void cargar_pcb(){
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
	estimacionInicial = config_get_string_value(config, "ESTIMACION_INICIAL");
	alfa = atoi(config_get_string_value(config, "ALFA"));
	gradoMultiprogramacionTotal = atoi(config_get_string_value(config, "GRADO_MULTIPROGRAMACION"));
	tiempoMaximoBloqueado = config_get_string_value(config, "TIEMPO_MAXIMO_BLOQUEADO");
	gradoMultiprogramacionActual = 0; //Arranca en 0 porque no hay procesos en memoria
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
     socketServidor = iniciar_servidor();
     log_info(logger, "Servidor listo para recibir al cliente");
     socketMemoria = crear_conexion(ipMemoria, puertoMemoria);
	 socketCpuDispatch = crear_conexion(ipCpu, puertoCpuDispatch);
	 //falta también las conexiones con cpu para interrupciones
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

t_log* iniciar_logger(void){
	return log_create("./kernel.log","KERNEL",true,LOG_LEVEL_INFO);
}

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
