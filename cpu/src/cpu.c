#include "funcionesCpu.h"

int direccionLogica = 0; //harcodeada: esto en realidad viene del pcb -> instrucciones -> (read,write,copy) -> parametro[0]
int nroTabla1erNivel = 0; //harcodeada: esto en realidad viene del pcb -> tabla_de_paginas

tlb* unaTlb;

int main(void) {
	//conexionConKernel();
	logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_DEBUG);
	inicializarConfiguraciones();
	conexionConMemoria();

	unaTlb = inicializarTLB();

	//Funciones de prueba
	reiniciarTLB();


	log_info(logger,"cantidad entradas %d", list_size(unaTlb->listaDeCampos));


	terminar_programa(conexion, logger, config);
}

int conexionConMemoria(void){

	// Creamos una conexión hacia el servidor
    conexion = crear_conexion(ipMemoria, puertoMemoria);
	log_info(logger,"Hola memoria, soy cpu");

	enviar_mensaje("Dame el tamanio de pag y entradas por tabla",conexion);

	t_list* listaQueContieneTamanioDePagYEntradas = list_create();
	t_list* listaQueContieneNroTabla2doNivel = list_create();
	int a = 1;
	while (a == 1) {
		int cod_op = recibir_operacion(conexion);

		switch (cod_op){
				case PAQUETE:
					listaQueContieneTamanioDePagYEntradas = recibir_paquete(conexion);

					leerTamanioDePaginaYCantidadDeEntradas(listaQueContieneTamanioDePagYEntradas);

					calculosDireccionLogica();

					paqueteEntradaTabla1erNivel(conexion);//1er acceso con esto memoria manda nroTabla2doNivel
					break;
				case PAQUETE2:

					listaQueContieneNroTabla2doNivel = recibir_paquete(conexion);//finaliza 1er acceso
					int nroTabla2doNivel = (int) list_get(listaQueContieneNroTabla2doNivel,0);
					log_info(logger,"Me llego el numero de tabla de segundoNivel que es % d",nroTabla2doNivel);

					paqueteEntradaTabla2doNivel(conexion); //2do acceso a memoria

					a = 0; //Para que salga del while
					break;
				default:
					log_warning(logger,"Operacion desconocida. No quieras meter la pata");
					break;
				}
		}

	return EXIT_SUCCESS;
}

void inicializarConfiguraciones(){
	config = config_create("cpu.config");

	ipMemoria = config_get_string_value(config,"IP_MEMORIA");
	puertoMemoria = config_get_int_value(config,"PUERTO_MEMORIA");

	cantidadEntradasTlb = config_get_int_value(config,"ENTRADAS_TLB");
	algoritmoReemplazoTlb = config_get_string_value(config,"REEMPLAZO_TLB");
	retardoDeNOOP = config_get_int_value(config,"RETARDO_NOOP");

	puertoDeEscuchaDispath = config_get_int_value(config,"PUERTO_ESCUCHA_DISPATCH");
	puertoDeEscuchaInterrupt = config_get_int_value(config,"PUERTO_ESCUCHA_INTERRUPT");
}

void calculosDireccionLogica(){
	numeroDePagina = floor(direccionLogica/ tamanioDePagina);
	entradaTabla1erNivel = floor(numeroDePagina / entradasPorTabla);
	entradaTabla2doNivel = numeroDePagina % entradasPorTabla;
	desplazamiento = direccionLogica - numeroDePagina * tamanioDePagina;
}

void leerTamanioDePaginaYCantidadDeEntradas(t_list* listaQueContieneTamanioDePagYEntradas){
	log_info(logger, "Me llegaron los siguientes valores:");

	tamanioDePagina = (int) list_get(listaQueContieneTamanioDePagYEntradas,0);
	entradasPorTabla = (int) list_get(listaQueContieneTamanioDePagYEntradas,1);

	log_info(logger,"tamanio de pagina: %d ",tamanioDePagina);
	log_info(logger,"entradas por tabla: %d ",entradasPorTabla);

}

//Inicializar para poder utilizar la cahce
tlb* inicializarTLB(){
	unaTlb = malloc(sizeof(tlb));
	unaTlb->listaDeCampos = list_create();

	generarListaCamposTLB(unaTlb);

	return unaTlb;
}

//Cargar lista de campos
void generarListaCamposTLB(tlb* unaTlb){

	for(int i = 0; i<cantidadEntradasTlb;i++){
			campoTLB* unCampo = malloc(sizeof(campoTLB));
			list_add(unaTlb->listaDeCampos,unCampo);
		}
}

//Reiniciar la TLB cuando se hace process switch
void reiniciarTLB(){
	list_clean(unaTlb->listaDeCampos);
	generarListaCamposTLB(unaTlb);

}

//Busqueda de pagina en la TLB
int chequeoDePagina(int numeroDePagina){
	campoTLB* unCampo = malloc(sizeof(campoTLB));

	for(int i=0;i < list_size(unaTlb->listaDeCampos);i++){
		unCampo = list_get(unaTlb->listaDeCampos,i);

			if(unCampo->nroDePagina == numeroDePagina){
				return unCampo->nroDeMarco; // se puede almacenar este valor en una variable y retornarlo fuera del for
			}
	}
	free(unCampo);

}
//En el caso en el que no este, tiene que ir a memoria y buscar la pagina y el marco y etc.
void reemplazoDeCampoEnTLB(){

}

void paqueteEntradaTabla1erNivel(int conexion){
	t_paquete* paquete = crear_paqueteEntradaTabla1erNivel();
	// Leemos y esta vez agregamos las lineas al paquete
	agregar_a_paquete(paquete,&entradaTabla1erNivel,sizeof(entradaTabla1erNivel));
	agregar_a_paquete(paquete,&nroTabla1erNivel,sizeof(nroTabla1erNivel));

	log_info(logger,"Le envio a memoria nro tabla 1er nivel y entrada de 1er nivel (que son %d y %d)",nroTabla1erNivel,entradaTabla1erNivel);
	enviar_paquete(paquete,conexion);
	eliminar_paquete(paquete);
}

void paqueteEntradaTabla2doNivel(int conexion){
	t_paquete* paquete = crear_paqueteEntradaTabla2doNivel();
	// Leemos y esta vez agregamos las lineas al paquete
	agregar_a_paquete(paquete,&entradaTabla2doNivel,sizeof(entradaTabla2doNivel));

	log_info(logger,"Le envio a memoria entrada de tabla de 2do nivel que es %d)",entradaTabla2doNivel);
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


void iterator(int value) {
	log_info(logger,"%d", value);
}
