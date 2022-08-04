#include "funcionesCpu.h"

//----------------------------- Para ser servidor de Kernel ------------------------------------
int iniciar_servidor(int tipoDePuerto) // puede ser interrupt o dispatch
{

	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;


	char* puerto= string_itoa(tipoDePuerto);// convierte el entero a string para el getaddrinfo

	getaddrinfo(IP_CPU, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(servinfo->ai_family,servinfo->ai_socktype, servinfo->ai_protocol);

	// Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);
	log_trace(logger, "Listo para escuchar al Kernel");

	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL); // acepto al cliente------------ ver esto----------------
	log_info(logger, "Se conecto el Kernel!");

	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		int valor = 0;
		memcpy(&valor, buffer+desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		list_add(valores, (void *)valor);
	}
	free(buffer);
	return valores;
}

t_pcb* recibir_pcb(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	//int identificador_length;

	buffer = recibir_buffer(&size, socket_cliente);
	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->instrucciones = list_create();
	int contadorInstrucciones;
	int i = 0;
		memcpy(&pcb->idProceso, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&pcb->tamanioProceso, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&contadorInstrucciones, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		while(i < contadorInstrucciones){
			instruccion* unaInstruccion = malloc(sizeof(instruccion));
			int identificador_length;
			memcpy(&identificador_length, buffer + desplazamiento, sizeof(int));
			desplazamiento+=sizeof(int);
			unaInstruccion->identificador = malloc(identificador_length);
			memcpy(unaInstruccion->identificador, buffer+desplazamiento, identificador_length);
			desplazamiento+=identificador_length;
			memcpy(unaInstruccion->parametros, buffer+desplazamiento, sizeof(int[2]));
			desplazamiento+=sizeof(int[2]);
			list_add(pcb -> instrucciones, unaInstruccion);
			i++;
		}
		memcpy(&pcb->programCounter, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&pcb->nroTabla1erNivel, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&pcb->estimacionRafaga, buffer + desplazamiento, sizeof(float));
		desplazamiento+=sizeof(float);
		//memcpy(&pcb->socket_cliente, buffer + desplazamiento, sizeof(int)); al pedo mepa
	free(buffer);
	return pcb;
}

//----------------------------- Para ser cliente de Memoria -------------------------------------------------
void enviar_mensaje(char* mensaje, int socket_cliente,int cod_op)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	if(socket_cliente == clienteKernel){
		paquete->codigo_operacion_kernel = MENSAJE_A_KERNEL;
	}else if(socket_cliente == conexionMemoria){
		paquete->codigo_operacion_memoria = MENSAJE_CPU_MEMORIA;
	}
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	if(clienteKernel){
		memcpy(magic + desplazamiento, &(paquete->codigo_operacion_kernel), sizeof(int));
	}else if(conexionMemoria){
		memcpy(magic + desplazamiento, &(paquete->codigo_operacion_memoria), sizeof(int));
	}

	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

int crear_conexion(char *ip, int puertoMemoria)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	char* puerto = string_itoa(puertoMemoria); // convierte el entero a string para el getaddrinfo

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family,server_info->ai_socktype, server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info);

	return socket_cliente;
}


void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}


t_paquete* crear_paquete(int cod_op) //entrada1erNivel paquete1 entrada2doNivel paquete2 dirFisicaYValor paquete3
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	if(clienteKernel){
		paquete->codigo_operacion_kernel = cod_op;
	}else if(conexionMemoria){
		paquete->codigo_operacion_memoria = cod_op;
	}

	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}
//ESTO ES PARA MANDAR UN PCB A KERNEL-------------------------------------------------------------------------------------
t_paquete* agregar_a_paquete_kernel_cpu(t_pcb* pcb,int cod_op)
{
	tamanioTotalIdentificadores = 0;
	contadorInstrucciones = 0;
	desplazamiento = 0;
	paquete = crear_paquete(cod_op);
	list_iterate(pcb->instrucciones, (void*) obtenerTamanioIdentificadores);
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanioTotalIdentificadores + contadorInstrucciones*sizeof(int[2]) + contadorInstrucciones*sizeof(int) + 6*sizeof(int) + sizeof(float));
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->idProceso), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->tamanioProceso), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &contadorInstrucciones, sizeof(int));
	desplazamiento+=sizeof(int);
	list_iterate(pcb->instrucciones, (void*) agregarInstruccionesAlPaquete);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->programCounter), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->nroTabla1erNivel), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->estimacionRafaga), sizeof(float));
	desplazamiento+=sizeof(float);
//	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->socket_cliente), sizeof(int)); No se para que esta
//	desplazamiento+=sizeof(int);
	paquete->buffer->size = desplazamiento;
	free(pcb);

	return paquete;
}

void obtenerTamanioIdentificadores(instruccion* unaInstruccion) {
	tamanioTotalIdentificadores += (strlen(unaInstruccion -> identificador)+1);
	contadorInstrucciones++;
}

void agregarInstruccionesAlPaquete(instruccion* unaInstruccion) {
	void* id = unaInstruccion->identificador;
	int longitudId = strlen(unaInstruccion->identificador)+1;
	memcpy(paquete->buffer->stream + desplazamiento, &longitudId, sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, id, longitudId);
	desplazamiento+=longitudId;
	memcpy(paquete->buffer->stream + desplazamiento, &(unaInstruccion->parametros), sizeof(int[2]));
	desplazamiento+=sizeof(int[2]);
	free(unaInstruccion->identificador);
	free(unaInstruccion);
}
//ESTO ES PARA MANDAR UN PCB A KERNEL--------------------------------------------------------------------------------------
void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}



op_code generarCode(char *inst){

        if(strcmp(inst,"NO_OP") == 0){
        return NO_OP1;}

        else if(strcmp(inst,"WRITE") == 0){
        return WRITE1;}

        else if(strcmp(inst,"READ") == 0){
        return READ1;}

        else if(strcmp(inst,"I/O") == 0){
        return IO1;}

        else if(strcmp(inst,"COPY") == 0){
        return COPY1;}

        else if(strcmp(inst,"EXIT") == 0){
        return EXIT1;}

        else
        printf("ERROR, RECIBI UNA INSTRUCCION CON ID INEXISTENTE (%s)\n", inst);
        exit(-1);
    }


static void* serializar_PCB_mas_int (t_pcb* pcbASerializar, size_t* tamanio_stream, uint32_t* elInt) {
	// leo PCB
	uint32_t id = pcbASerializar -> idProceso;
	uint32_t tamProceso = pcbASerializar -> tamanioProceso;
	t_list* listaInstrucciones = pcbASerializar -> instrucciones;
	t_estado estadoProceso = pcbASerializar -> estado;
	uint32_t programCounter = pcbASerializar -> programCounter;
	uint32_t tablaPaginas = pcbASerializar -> nroTabla1erNivel;
	uint32_t estimacionRafaga = pcbASerializar -> estimacionRafaga;

	// calculo tamaños
	size_t tamanio_id = sizeof(uint32_t);
	size_t tamanio_tamProceso = sizeof(uint32_t);
	size_t tamanio_tamLista = sizeof(size_t);
	size_t tamLista = list_size(listaInstrucciones) * sizeof(instruccion);
	size_t tamanio_estado = sizeof(t_estado);
	size_t tamanio_PC = sizeof(uint32_t);
	size_t tamanio_tablaPaginas = sizeof(uint32_t);
	size_t tamanio_estimacionRafaga = sizeof(uint32_t);
	size_t tamanio_tamPayload = sizeof(size_t);
	size_t tamanio_elInt = sizeof(uint32_t);

	//printf("El estado en serializar es %s\n", generar_string_de_estado(estadoProceso));

	size_t tamPayload = tamanio_id + tamanio_tamProceso + tamanio_tamLista + tamLista +
						tamanio_estado + tamanio_PC + tamanio_tablaPaginas +
						tamanio_estimacionRafaga + tamanio_elInt;
	*tamanio_stream = tamPayload + tamanio_tamPayload;

	// creo el stream
	void* stream = malloc(*tamanio_stream);
	instruccion* instAGuardar;
	int i=0;
	op_code cop;			//HACER TODOS
	uint32_t param0;		//LOS FREE
	uint32_t param1;		//CUANDO TERMINA LA PCB

	// serializo el tamaño del payload
	memcpy(stream, &tamPayload, tamanio_tamPayload);

	// serializo el payload en si
	memcpy(stream + tamanio_tamPayload, &id, tamanio_id);
	memcpy(stream + tamanio_tamPayload + tamanio_id, &tamProceso, tamanio_tamProceso);
	memcpy(stream + tamanio_tamPayload + tamanio_id + tamanio_tamProceso, &tamLista, tamanio_tamLista);
	while (i < list_size(listaInstrucciones)) {
		instAGuardar = list_get(listaInstrucciones,i);
		cop = generarCode(instAGuardar -> identificador);
		param0 = (instAGuardar -> parametros[0]);
		param1 = instAGuardar -> parametros[1];
		memcpy(stream + tamanio_tamPayload + tamanio_id + tamanio_tamProceso + tamanio_tamLista + i*(sizeof(instruccion)), &cop, sizeof(op_code));
		memcpy(stream + tamanio_tamPayload + tamanio_id + tamanio_tamProceso + tamanio_tamLista + i*(sizeof(instruccion)) + sizeof(op_code), &param0, sizeof(uint32_t));
		memcpy(stream + tamanio_tamPayload + tamanio_id + tamanio_tamProceso + tamanio_tamLista + i*(sizeof(instruccion)) + sizeof(op_code) + sizeof(uint32_t), &param1, sizeof(uint32_t));
		i++;
	}
	memcpy(stream + tamanio_tamPayload + tamanio_id + tamanio_tamProceso + tamanio_tamLista + i*sizeof(instruccion), &estadoProceso, tamanio_estado);
	memcpy(stream + tamanio_tamPayload + tamanio_id + tamanio_tamProceso + tamanio_tamLista + i*sizeof(instruccion) + tamanio_estado, &programCounter, tamanio_PC);
	memcpy(stream + tamanio_tamPayload + tamanio_id + tamanio_tamProceso + tamanio_tamLista + i*sizeof(instruccion) + tamanio_estado + tamanio_PC, &tablaPaginas, tamanio_tablaPaginas);
	memcpy(stream + tamanio_tamPayload + tamanio_id + tamanio_tamProceso + tamanio_tamLista + i*sizeof(instruccion) + tamanio_estado + tamanio_PC + tamanio_tablaPaginas, &estimacionRafaga, tamanio_estimacionRafaga);
	memcpy(stream + tamanio_tamPayload + tamanio_id + tamanio_tamProceso + tamanio_tamLista + i*sizeof(instruccion) + tamanio_estado + tamanio_PC + tamanio_tablaPaginas + tamanio_estimacionRafaga, elInt, tamanio_elInt);

	return stream;
}


bool send_PCB_mas_int (int conexion, t_pcb* pcbASerializar, uint32_t elInt) {
	size_t tamanio_stream;
	void* stream = serializar_PCB_mas_int(pcbASerializar, &tamanio_stream, &elInt);

	if (send(conexion, stream, tamanio_stream, 0) != tamanio_stream) {
		free(stream);
		printf("\n\n\n(send_pcb_mas_int) NO ENVIADO (SOCKET %d)\n\n\n", conexion);
		return false;
	}

	free(stream);
	return true;
}

