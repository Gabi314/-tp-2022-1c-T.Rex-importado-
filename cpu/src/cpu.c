#include "funcionesCpu.h"
#include "tlb.h"

int nroTabla1erNivel = 0; //harcodeada: esto en realidad viene del pcb -> tabla_de_paginas
t_pcb* unPcb;  // viene de kernel
int hayInstrucciones;
int bloqueado = 0;
int main(void) {
	logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_DEBUG);
	inicializarConfiguraciones();
	//conexionConKernel(); recibo el pcb
	unPcb = malloc(sizeof(t_pcb));
	unPcb->programCounter = 0;

	tlb = inicializarTLB();
	conexionConMemoria();

	//lo que verdadermente se hace: ejecutar(buscarInstruccionAEjecutar(unPCB))

	instruccion* instruccion0 = malloc(sizeof(instruccion));
	instruccion0->identificador = "WRITE";
	instruccion0->parametros[0] = 0;
	instruccion0->parametros[1] = 1000;

	instruccion* instruccion1 = malloc(sizeof(instruccion));
	instruccion1->identificador = "WRITE";
	instruccion1->parametros[0] = 4;
	instruccion1->parametros[1] = 1001;

	instruccion* instruccion2 = malloc(sizeof(instruccion));
	instruccion2->identificador = "WRITE";
	instruccion2->parametros[0] = 260;
	instruccion2->parametros[1] = 2000;

	instruccion* instruccion3 = malloc(sizeof(instruccion));
	instruccion3->identificador = "COPY";
	instruccion3->parametros[0] = 256;
	instruccion3->parametros[1] = 260;

//	instruccion* instruccion4 = malloc(sizeof(instruccion));
//	instruccion4->identificador = "I/O";
//	instruccion4->parametros[0] = 6000;

	instruccion* instruccion5 = malloc(sizeof(instruccion));
	instruccion5->identificador = "WRITE";
	instruccion5->parametros[0] = 512;
	instruccion5->parametros[1] = 3000;

	instruccion* instruccion6 = malloc(sizeof(instruccion));
	instruccion6->identificador = "EXIT";

	unPcb->instrucciones = list_create();

	list_add_in_index(unPcb->instrucciones,0,instruccion0);
	list_add_in_index(unPcb->instrucciones,1,instruccion1);
	list_add_in_index(unPcb->instrucciones,2,instruccion2);
	list_add_in_index(unPcb->instrucciones,3,instruccion3);
//	list_add_in_index(unPcb->instrucciones,4,instruccion4);
	list_add_in_index(unPcb->instrucciones,4,instruccion5);
	list_add_in_index(unPcb->instrucciones,5,instruccion6);

	//list_iterate(unPcb->instrucciones, ejecutar);
	hayInstrucciones = 1;
	while(hayInstrucciones){
//		if(bloqueado){
//			nroTabla1erNivel = 1;//viene otro process
//		}
		ejecutar(buscarInstruccionAEjecutar(unPcb));
	}

	log_info(logger,"Termino cpu");
	//terminar_programa(conexion, logger, config);
}

void inicializarConfiguraciones(){
	config = config_create("cpu.config");// ver bien como recibir los path de config por parametros

	ipMemoria = config_get_string_value(config,"IP_MEMORIA");
	puertoMemoria = config_get_int_value(config,"PUERTO_MEMORIA");

	cantidadEntradasTlb = config_get_int_value(config,"ENTRADAS_TLB");
	algoritmoReemplazoTlb = config_get_string_value(config,"REEMPLAZO_TLB");
	retardoDeNOOP = config_get_int_value(config,"RETARDO_NOOP");

	puertoDeEscuchaDispath = config_get_int_value(config,"PUERTO_ESCUCHA_DISPATCH");
	puertoDeEscuchaInterrupt = config_get_int_value(config,"PUERTO_ESCUCHA_INTERRUPT");
}

int conexionConMemoria(void){

	// Creamos una conexión hacia el servidor
    conexionMemoria = crear_conexion(ipMemoria, puertoMemoria);
	log_info(logger,"Hola memoria, soy cpu");

	enviar_mensaje("Dame el tamanio de pag y entradas por tabla",conexionMemoria,MENSAJE_CPU_MEMORIA);//MENSAJE_PEDIDO_CPU

	t_list* listaQueContieneTamanioDePagYEntradas = list_create();

	int cod_op = recibir_operacion(conexionMemoria);

	if(cod_op == TAM_PAGINAS_Y_CANT_ENTRADAS){//TAM_PAGINAS_Y_CANT_ENTRADAS
		listaQueContieneTamanioDePagYEntradas = recibir_paquete(conexionMemoria);
	}

	leerTamanioDePaginaYCantidadDeEntradas(listaQueContieneTamanioDePagYEntradas);

	return EXIT_SUCCESS;
}

int accederAMemoria(int marco){

	log_info(logger,"La pagina no se encuentra en tlb, se debera acceder a memoria(tabla de paginas)");
	enviarEntradaTabla1erNivel();//1er acceso con esto memoria manda nroTabla2doNivel

	int seAccedeAMemoria = 1;

	while (seAccedeAMemoria == 1) {
		int cod_op = recibir_operacion(conexionMemoria);

		t_list* listaQueContieneNroTabla2doNivel = list_create();
		t_list* listaQueContieneMarco = list_create();

		switch (cod_op){
			case PRIMER_ACCESO://PRIMER_ACCESO

				listaQueContieneNroTabla2doNivel = recibir_paquete(conexionMemoria);//finaliza 1er acceso
				int nroTabla2doNivel = (int) list_get(listaQueContieneNroTabla2doNivel,0);
				log_info(logger,"Me llego el numero de tabla de segundoNivel que es % d",nroTabla2doNivel);

				enviarEntradaTabla2doNivel(); //2do acceso a memoria
				break;
			case SEGUNDO_ACCESO://SEGUNDO_ACCESO
				listaQueContieneMarco = recibir_paquete(conexionMemoria);//Finaliza el 2do acceso recibiendo el marco
				marco = (int) list_get(listaQueContieneMarco,0);

				log_info(logger,"Me llego el marco que es %d",marco);

				if(list_size(tlb) < cantidadEntradasTlb){
					agregarEntradaATLB(marco,numeroDePagina);
					sleep(1);// Para que espere haya 1 seg de diferencia( a veces pasa que se agregan en el mismo seg y jode los algoritmos)
				}else{
					algoritmosDeReemplazoTLB(numeroDePagina,marco);
				}
				seAccedeAMemoria = 0;//salga del while
				break;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				seAccedeAMemoria = 0;//salga del while
				break;
		}
	}

	return marco;
}

//------------------CICLO DE INSTRUCCION--------------------------
instruccion* buscarInstruccionAEjecutar(t_pcb* unPCB){//FETCH

	instruccion* unaInstruccion = list_get(unPCB->instrucciones,unPCB->programCounter);
	unPCB->programCounter++;

	return unaInstruccion;
}

//void decode(instruccion* unaInstruccion){//capaz esta al pedo
//	if(! strcmp(unaInstruccion->identificador,"COPY")){
//		buscarDireccionFisica(unaInstruccion->parametros[1]);//la segunda dir logica
//		//ejecutar(unaInstruccion);
//	}else{
//		//ejecutar(unaInstruccion);
//	}
//}

void ejecutar(instruccion* unaInstruccion){
	if(! strcmp(unaInstruccion->identificador,"WRITE")){
		log_info(logger,"----------------EXECUTE WRITE----------------");

		int marco = buscarDireccionFisica(unaInstruccion->parametros[0]);// se podria enviar la pagina para saber en memoria cual se modifica
		enviarDireccionFisica(marco,desplazamiento,0);//con 0 envia la dir fisica para escribir
		enviarValorAEscribir(unaInstruccion->parametros[1]);

		int cod_op = recibir_operacion(conexionMemoria);

		if(cod_op == MENSAJE_CPU_MEMORIA){// Recibe que se escribio correctamente el valor en memoria
			recibir_mensaje(conexionMemoria);
			log_info(logger,"----------------FINALIZA WRITE----------------\n");
		}

	}else if(! strcmp(unaInstruccion->identificador,"READ")){
		log_info(logger,"----------------EXECUTE READ----------------");

		int marco = buscarDireccionFisica(unaInstruccion->parametros[0]);
		enviarDireccionFisica(marco,desplazamiento,1);//con 1 enviar dir fisica para leer

		log_info(logger,"----------------FINALIZA READ----------------\n");

	}else if(! strcmp(unaInstruccion->identificador,"COPY")){
		log_info(logger,"----------------EXECUTE COPY----------------");
		int marcoDeOrigen = buscarDireccionFisica(unaInstruccion->parametros[1]);//DEBUGGEAR
		int desplazamientoOrigen = desplazamiento;

		int marcoDeDestino = buscarDireccionFisica(unaInstruccion->parametros[0]);
		int desplazamientoDestino = desplazamiento;

		enviarDireccionesFisicasParaCopia(marcoDeDestino,desplazamientoDestino,marcoDeOrigen,desplazamientoOrigen);
		log_info(logger,"----------------FINALIZA COPY----------------\n");

	}else if(! strcmp(unaInstruccion->identificador,"NO_OP")){
		sleep(retardoDeNOOP/1000);// miliseg

	}else if(! strcmp(unaInstruccion->identificador,"I/O")){

		log_info(logger,"----------------I/O-----------------------");
		//sleep(unaInstruccion->parametros[0]/1000);
		//enviar_mensaje("Se suspende el proceso",conexionMemoria,MENSAJE);//Se envia pcb a kernel solamente
		//enviarPcb(unPcb);
		bloqueado = 1;
		// luego kernel le avisa a memoria que se suspende
		reiniciarTLB();

		log_info(logger,"----------------FIN DE I/O----------------");

		//log_info(logger,"Se reanuda el proceso\n");


	}else if(! strcmp(unaInstruccion->identificador,"EXIT")){
		// enviar pcb actualizado finaliza el proceso
		//enviarPcb(unPcb);
		log_info(logger,"Finalizo el proceso ");
		hayInstrucciones = 0;
	}
}


int buscarDireccionFisica(int direccionLogica){
	calculosDireccionLogica(direccionLogica);
	int marco = chequearMarcoEnTLB(numeroDePagina);

	if (marco == -1){
		marco = accederAMemoria(marco);
	}

	return marco;

}


void calculosDireccionLogica(int direccionLogica){
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
	log_info(logger,"entradas por tabla: %d \n",entradasPorTabla);

}


void enviarEntradaTabla1erNivel(){//pasar entrada y nroTabla1ernivel
	t_paquete* paquete = crear_paquete(PRIMER_ACCESO);
	// Leemos y esta vez agregamos las lineas al paquete
	agregar_a_paquete(paquete,&entradaTabla1erNivel,sizeof(entradaTabla1erNivel));
	agregar_a_paquete(paquete,&nroTabla1erNivel,sizeof(nroTabla1erNivel));

	log_info(logger,"Le envio a memoria nro tabla 1er nivel y entrada de 1er nivel (que son %d y %d)",nroTabla1erNivel,entradaTabla1erNivel);
	enviar_paquete(paquete,conexionMemoria);
	eliminar_paquete(paquete);
}

void enviarEntradaTabla2doNivel(){//pasar la entrada
	t_paquete* paquete = crear_paquete(SEGUNDO_ACCESO);
	// Leemos y esta vez agregamos las lineas al paquete
	agregar_a_paquete(paquete,&entradaTabla2doNivel,sizeof(entradaTabla2doNivel));

	log_info(logger,"Le envio a memoria entrada de tabla de 2do nivel que es %d",entradaTabla2doNivel);
	enviar_paquete(paquete,conexionMemoria);
	eliminar_paquete(paquete);
}

void enviarDireccionFisica(int marco, int desplazamiento,int leer){
	t_paquete* paquete = crear_paquete(READ);

	agregar_a_paquete(paquete,&marco,sizeof(marco));
	agregar_a_paquete(paquete,&desplazamiento,sizeof(desplazamiento));
	agregar_a_paquete(paquete,&leer,sizeof(leer));

	log_info(logger,"Le envio a memoria direccion fisica: Marco:%d y Offset: %d",marco,desplazamiento);
	enviar_paquete(paquete,conexionMemoria);
	eliminar_paquete(paquete);
}

void enviarValorAEscribir(uint32_t valorAEscribir){
	t_paquete* paquete = crear_paquete(WRITE);

	agregar_a_paquete(paquete,&valorAEscribir,sizeof(uint32_t));

	log_info(logger,"Le envio a memoria el valor a escribir %u",valorAEscribir);
	enviar_paquete(paquete,conexionMemoria);
	eliminar_paquete(paquete);
}

void enviarDireccionesFisicasParaCopia(int marcoDeDestino,int desplazamientoDestino,int marcoDeOrigen,int desplazamientoOrigen){
	t_paquete* paquete = crear_paquete(COPY);

	agregar_a_paquete(paquete,&marcoDeDestino,sizeof(int));
	agregar_a_paquete(paquete,&desplazamientoDestino,sizeof(int));
	agregar_a_paquete(paquete,&marcoDeOrigen,sizeof(int));
	agregar_a_paquete(paquete,&desplazamientoOrigen,sizeof(int));

	log_info(logger,"Le envio a memoria para que copie de dir fisica1: (marco: %d offset: %d) a dir fisica2: (marco: %d offset: %d)",marcoDeOrigen,desplazamientoOrigen,marcoDeDestino,desplazamientoDestino);
	enviar_paquete(paquete,conexionMemoria);
	eliminar_paquete(paquete);
}

void enviarPcb(t_pcb* unPcb){

	paquete = agregar_a_paquete_kernel_cpu(unPcb);

	log_info(logger,"Envio el pcb actualizado a Kernel");
	enviar_paquete(paquete,clienteKernel);
	eliminar_paquete(paquete);
}

void terminar_programa(int conexionMemoria, t_log* logger, t_config* config){
	/* Y por ultimo, hay que liberar lo que utilizamos (conexionMemoria, log y config)
	  con las funciones de las commons y del TP mencionadas en el enunciado */
	log_destroy(logger);
	config_destroy(config);

	liberar_conexion(conexionMemoria);
}


//generico ver bien que recibe
//int conexionConKernel(void){
//	logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_DEBUG);
//
//	int server_fd = iniciar_servidor();
//	log_info(logger, "Cpu listo para recibir a Kernel");
//	int cliente_fd = esperar_cliente(server_fd);
//
//	t_list* lista;
//	while (1) {
//		int cod_op = recibir_operacion(cliente_fd);
//		switch (cod_op) {
//		case MENSAJE:
//			recibir_mensaje(cliente_fd);
//			break;
//		case PAQUETE:
//			lista = recibir_paquete(cliente_fd);
//			log_info(logger, "Me llegaron los siguientes valores:\n");
//			list_iterate(lista, (void*) iterator);
//			break;
//		case -1:
//			log_error(logger, "Kernel se desconecto. Terminando conexion");
//			return EXIT_FAILURE;
//		default:
//			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
//			break;
//		}
//	}
//	return EXIT_SUCCESS;
//}

