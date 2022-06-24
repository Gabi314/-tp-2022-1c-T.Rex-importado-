#include "funcionesMemoria.h"

int marcosTotales = 0;
int pid = 0;//lo hardcodeo
int entrada = 0;
t_list* listaT1nivel;

int main(void) {
	logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO);
	crearConfiguraciones();

	listaT1nivel = list_create();
	inicializarEstructuras();
	//pid++;
	//inicializarEstructuras(); preguntar en el grupo por que devuelve el pid 1 en posicion 0

	t_primerNivel* unaTablaDe1erNivel = malloc(sizeof(unaTablaDe1erNivel));
	unaTablaDe1erNivel = list_get(listaT1nivel,0);

	log_info(logger,"El pid es: %d",unaTablaDe1erNivel->pid);
	free(unaTablaDe1erNivel);

	log_info(logger,"El nro de tabla asociado al pid %d es %d",pid,buscarNroPaginaDe1erNivel(pid));


}

void crearConfiguraciones(){
	t_config* config = config_create("memoria.config");

	tamanioDeMemoria = config_get_int_value(config,"TAM_MEMORIA");
	tamanioDePagina = config_get_int_value(config,"TAM_PAGINA");

	entradasPorTabla = config_get_int_value(config,"ENTRADAS_POR_TABLA");
	retardoMemoria = config_get_int_value(config,"RETARDO_MEMORIA");
	algoritmoDeReemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
	marcosPorProceso = config_get_int_value(config,"MARCOS_POR_PROCESO");
	retardoSwap = config_get_int_value(config,"RETARDO_SWAP");

}

void inicializarEstructuras(){
	t_primerNivel* tablaPrimerNivel = malloc(sizeof(tablaPrimerNivel));

	int contadorDeMarcos = 0;
	tablaPrimerNivel->pid = pid;
	tablaPrimerNivel->tablasDeSegundoNivel = list_create();

	for(int i=0;i<entradasPorTabla;i++){

		t_segundoNivel* tablaDeSegundoNivel = malloc(sizeof(tablaDeSegundoNivel));
		tablaDeSegundoNivel->marco = contadorDeMarcos;
		//list_add(listaT2nivel,tablaDeSegundoNivel); No seria necesario por ahora

		list_add(tablaPrimerNivel->tablasDeSegundoNivel, tablaDeSegundoNivel);
		contadorDeMarcos++;


		free(tablaDeSegundoNivel);
	}


	list_add(listaT1nivel,tablaPrimerNivel);

	free(tablaPrimerNivel);

	escribirEnSwap(pid);
}

void escribirEnSwap(int pid){

	char* nombreArchivo = string_new();

	char* pidString = string_itoa(pid);
	nombreArchivo = pidString;

	string_append(&nombreArchivo,".swap");

	FILE* archivoSwap = fopen(nombreArchivo, "a+");

	fclose(archivoSwap);

}


int buscarNroPaginaDe1erNivel(int pid){
	t_primerNivel* unaTablaDe1erNivel = malloc(sizeof(unaTablaDe1erNivel));

	for(int i=0;i < list_size(listaT1nivel);i++){

		unaTablaDe1erNivel = list_get(listaT1nivel,i);

		if(unaTablaDe1erNivel->pid == pid){
			return i;
		}
	}
	return 4;//Puse 4 para que no me confunda si retorno 0
	free(unaTablaDe1erNivel);
}


int conexionConCpu(void){


	int server_fd = iniciar_servidor();
	log_info(logger, "Memoria lista para recibir a Cpu o Kernel");
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
			log_error(logger, "Se desconecto el cliente. Terminando conexion");
			return EXIT_FAILURE;
		default:
			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	return EXIT_SUCCESS;
}

void iterator(char* value) {
	log_info(logger,"%s", value);
}
