#include "funcionesCpu.h"

int main(void) {
	//conexionConKernel();
	conexionConMemoria();
}

int conexionConKernel(void){
	logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_DEBUG);

	int server_fd = iniciar_servidor();
	log_info(logger, "Cpu listo para recibir a Kernel");
	int cliente_fd = esperar_cliente(server_fd);

	t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(cliente_fd);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(cliente_fd);
			break;
		case PAQUETE:
			lista = recibir_paquete(cliente_fd);
			log_info(logger, "Me llegaron los siguientes valores:\n");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			log_error(logger, "Kernel se desconecto. Terminando conexion");
			return EXIT_FAILURE;
		default:
			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	return EXIT_SUCCESS;
}

void conexionConMemoria(void){

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
}

t_log* iniciar_logger(void){
	return log_create("./cpu.log","CPU",true,LOG_LEVEL_INFO);
}

t_config* iniciar_config(void){
	return config_create("cpu.config");
}

void paquete(int conexion, char* pedido){

	t_paquete* paquete = crear_paquete();

	// Leemos y esta vez agregamos las lineas al paquete
    agregar_a_paquete(paquete,pedido,strlen(pedido) + 1);
    //free(pcb); tira terrible error

    enviar_paquete(paquete,conexion);
    eliminar_paquete(paquete);
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
