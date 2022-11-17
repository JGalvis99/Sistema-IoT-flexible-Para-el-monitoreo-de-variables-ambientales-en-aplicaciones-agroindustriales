/**
 * \file
 *          Code of the WSN part of the sistem 
 *          "Sistema IoT flexible para el monitoreo 
 *          de variables ambientales en aplicaciones 
 *          agroindustriales"
 * \authors
 *          Geuseppe Segrera <se.geuseppe@javeriana.edu.co>
 *          Javier Galvis    <javieragalvis@javeriana.edu.co.se>
 *          Daniel Castro    <castroda@javeriana.edu.co.se>
 */

////////////////////////////
////////  LIBRERIAS ////////
////////////////////////////

#include "wsn_funtions.h"

////////////////////////////
////////  FUNCIONES ////////
////////////////////////////

void llenar_beacon(struct beacon *b, linkaddr_t id, int16_t rssi_p)
{
  b->rssi_p = rssi_p;
  linkaddr_copy(&b->id, &id);
}



void llenar_mensaje(struct unicast_msg_arry *b, linkaddr_t id, char *msg_send)
{
  b->strPtr[0] = &msg_send;
  linkaddr_copy(&b->from_id, &id);
}


///// FUNCIONES Nary/////

int forwarder;

//init_nodes(node_arr, 30);

MEMB(node_NT_mem, struct node_NT, 30);

void init_nodes(node_NT *n[], int num_nodes) {

	int i;
	int j = 1;
	for (i = 0; i < num_nodes; i++) {
		// itera en la direccion del array
		n[i] = new_node(j++);
	}
}

node_NT *new_node(int id) {

	node_NT *new_node;
  new_node = memb_alloc(&node_NT_mem);

	if (new_node != NULL) {
		new_node->sibling = NULL;
		new_node->child = NULL;
    new_node->parent = NULL;
		new_node->id = id;

	}else{
    printf("ERROR: we could not allocate a new entry for <<Nary_Nodes>> in tree_rssi\n");
  }

  return new_node;

}

void reset_nodes(node_NT *n[], int num_nodes){
  int i;
	int j = 1;
	for (i = 0; i < num_nodes; i++) {
		// itera en la direccion del array
    n[i]->sibling = NULL;
		n[i]->child = NULL;
    n[i]->parent = NULL;
	}
}


node_NT  *add_sibling(node_NT  *n, node_NT  *new_n) {
	if (n == NULL)
		return NULL;

  if(n == new_n)
  return NULL;

  printf("Adding sibling,main:%d,sibling:%d\n",n->id,new_n->id);

	while (n->sibling) {//Mientras tenga hermano
		// Recorre la lista de hermanos
		if (new_n->id == n->sibling->id) {//
			// Si encuentra el id del nuevo nodo en el siguiente hermano
			// se valida si el siguiente nodo tiene hermanos
			// if(n->sibling->sibling != NULL) {
			//}
			printf("SE REEMPLAZA NODO EXISTENTE\n");
      //printf("nuevo hermano:%d,%d\n",new_n->sibling->id,n->sibling->sibling->id);
			new_n->sibling = n->sibling->sibling; // Se agrega el hermano actual al nuevo nodo
      //printf("nuevo hermano 2:%d,%d\n",new_n->id,n->sibling->id);

      return (n->sibling = new_n),add_parent(new_n, n->parent);
			//return (n->sibling = new_n),(new_n->parent = n->parent);// Reemplaza el nodo existente por el nuevo
		}
		n = n->sibling;
	}
  //printf("nuevo hermano 3:%d,%d\n",new_n->id,n->sibling->id);

  return (n->sibling = new_n),add_parent(new_n, n->parent);
	//return (n->sibling = new_n),(new_n->parent = n->parent); // new_node()
}

node_NT  *add_child_v2(node_NT  *n, node_NT  *new_n) {
	if (n == NULL)
		return NULL;

    printf("Adding child,main:%d,child:%d\n",n->id,new_n->id);

  //Tiene como hermano al nuevo hermano
  if(new_n->id == n->sibling->id)
    n->sibling = NULL;//Eliminar al hermano

	if (n->child) {
		// Si hay un hijo
		// if(n->id == 1) { // si es raiz entonces
		//	return add_sibling(n->child, new_n); //n->child
		// } else {
		return add_sibling(n->child, new_n); // n->child
		//}
	} else {

    return (n->child = new_n),add_parent(new_n, n);
		//return (n->child = new_n),(new_n->parent = n); // new_node()

	}
}

node_NT  *add_parent(node_NT  *n, node_NT  *new_n) {

  //Condiciones de prevenciòn//
  //Si trata de agregarse a si mismo como padre
  if(n == new_n)
  return NULL;
  //Si trata de agregarse a su hijo como padre
  if(n->child == new_n)
  return (n->child=NULL),(n->parent = new_n);
  //Si trata de agregar a su hermano como padre
  if(n->sibling == new_n)
  return (n->sibling=NULL),(n->parent = new_n);


  return n->parent = new_n;
}

int finish_search=0;
int search_forwarder(node_NT *n, int id_dest) {
  finish_search=0;
	forwarder=0;
	//printf("nodo de busqueda:%d\n",n->id);
	while (n != NULL) {
		//printf("Searching\n");
		if(n->child != NULL){//Si tiene hijos
			//printf("nodo de busqueda:%d\n",n->id);//Nodo en el que se esta buscando
			if( n->child->id == id_dest ){//Si el hijo es el destino
				forwarder=n->id;
				//printf("Has Found\n");
				finish_search=1;
			}else{//Si el hijo no es el destino
				n = n->child;//Ir al hijo
				if( n->id == id_dest ){//Si el hermano es el destino
					forwarder=n->id;
					//printf("Has Found\n");
					finish_search=1;
					break;
				}else{//Si el hermano no es el destino
					if(n->child != NULL){//si tiene desendientes
						//printf("Searching desendents of %d\n",n->id);
						search_forwarder_RAW(n->child, id_dest);//Buscar en la desendencia del hijo
					}
					if(finish_search){//Si se encontro en la descendencia
						forwarder=n->id;
						//printf("finish_search_true\n");
						break;//Salir de el bucle de hijos del nodo de origen
					}else{//Si no se encontro en la descendencia
						//printf("No se encontro en la descendencia\n");
					}//Fin de Si no se encontro en la descendencia
				}//Fin de Si el hermano no es el destino
				while(n->sibling != NULL){//Hasta que el hijo no tenga hermanos(Hasta que el origen no tenga mas hijos)
					n=n->sibling;//Ir al hermano(siguiente hijo del origen)
					if( n->id == id_dest ){//Si el hermano es el destino
						forwarder=n->id;
						//printf("Has Found\n");
						finish_search=1;
						break;
					}else{//Si el hermano no es el destino
						if(n->child != NULL){//si tiene desendientes
							//printf("Searching desendents of %d\n",n->id);
							search_forwarder_RAW(n->child, id_dest);//Buscar en la desendencia del hijo
						}
						if(finish_search){//Si se encontro en la descendencia
							forwarder=n->id;
							//printf("finish_search_true\n");
							break;//Salir de el bucle de hijos del nodo de origen
						}else{//Si no se encontro en la descendencia
							//printf("No se encontro en la descendencia\n");
						}//Fin de Si no se encontro en la descendencia
					}//Fin de Si el hermano no es el destino
				}//Otra vuelta de Hasta que el hijo no tenga hermanos(Hasta que el origen no tenga mas hijos)
			}
		}else{
			break;
		}
		break;
	}
	return forwarder;
}

void search_forwarder_RAW(node_NT *n, int id_dest) {
	//printf("nodo de busqueda:%d\n",n->id);
	while (n != NULL) {
		//printf("Searching\n");
		if (  n->id == id_dest){
			//printf("Has Found\n");
			finish_search=1;
			break;
		}
		if(n->child != NULL){
			if (  n->child->id == id_dest || (n->child->sibling != NULL && n->child->sibling->id == id_dest)) { // si el hermano o el hijo del nodo actual es el destino entonces retorne el id actual
				// SI encontramos el nodo entonces
				//printf("Has Found\n");
				finish_search=1;
				break;
			}
			// Si tiene hijos
			//printf("Has Children\n");
			search_forwarder_RAW(n->child, id_dest);//Busca si el hijo tiene el destino entre sus hijos
			if(finish_search){
				break;
			}
		}else{
			//printf("Has No Children\n");
		}
		if (n->sibling!= NULL){
				n = n->sibling;
				//printf("Has Sibling:%d\n", n->id);
		}else{
			//printf("Has No Sibling\n");
			break;
		}
	}
}

char routing_msg_V3[MSG_LENGHT];//Arreglo de serialización
char *msg_form;

//////Función para serializar sin llamarse a si misma////////
char *serializeV3(node_NT *level_node){
  routing_msg_V3[0]='N';
  routing_msg_V3[1]='T';

	int finish=0;
	int index_msg_V3=2;//longitud del mensaje de serialización
	//Hasta que no tenga hijos imprimir los ids
	while (level_node != NULL) {
		////Añadir id al mensaje////

		routing_msg_V3[index_msg_V3++]=char_cast(level_node->id);
		////Añadir id al mensaje////
		if(level_node->child != NULL){//Si tiene hijo
      //printf("Nodo:%d,Hijo:%d\n",level_node->id,level_node->child->id);
			level_node = level_node->child;//Se redirige al hijo
		}else{//Si no tiene hijo
			////Añadir ) al mensaje////

			routing_msg_V3[index_msg_V3++]=')';
			////Añadir ) al mensaje////
			if(level_node->sibling != NULL){//Si tiene hermano
        //printf("Nodo:%d,Hermano:%d\n",level_node->id,level_node->sibling->id);
				level_node = level_node->sibling;//Se redirige al hermano
        //printf("Serialize flag 1:%d\n",level_node->id);
			}else{//Si no tiene ni hermanos ni hijos pasar al padre

				////Añadir ) al mensaje////
				routing_msg_V3[index_msg_V3++]=')';
				////Añadir ) al mensaje////
				if(level_node->parent != NULL){
					level_node = level_node->parent;//Se redirige al padre
					if(char_cast(level_node->id) != routing_msg_V3[2]){//Si no ha terminado
						if(level_node->sibling != NULL){//Si tiene hermano
							level_node = level_node->sibling;//Se redirige al hermano
						}else{
							while(level_node->sibling == NULL){//Mientras no tenga hermano
								////Añadir ) al mensaje////
								routing_msg_V3[index_msg_V3++]=')';
								////Añadir ) al mensaje////
								level_node = level_node->parent;//Se redirige al padre
								if(char_cast(level_node->id) == routing_msg_V3[2]){//Si ha terminado
									finish=1;
									break;
								}
							}
							if(!finish){//Si no ha terminado
								level_node = level_node->sibling;//Se redirige al hermano
							}else{
								break;
							}
						}
					}else{
						break;
					}
				}else{
					break;
				}
			}
		}
	}
	routing_msg_V3[index_msg_V3++]='\0';//Finalizar mensaje
	//return routing_msg_V3;
	//printf("%s\n",msg_form);
	return routing_msg_V3;
}
//////Función para serializar sin llamarse a si misma////////

/////Convertir int en char/////
char char_cast(int num){
	char caract=num+'0';
	return caract;
}
/////Convertir int en char/////

/////Convertir char en int/////
int int_cast(char caract){
	int num=caract-'0';
	return num;
}
/////Convertir char en int/////


//////Función para deserializar//////
void deserialize(char msg[]){
  //printf("mensaje funcion:%s\n",msg);
	int msg_lenght=(int)strlen(msg)-1;
	int node_id_1;
	int node_id_2;
	int count1=2;
	int count2=3;
	int finish=0;

	node_id_1=int_cast(msg[count1])-1;
	node_id_2=int_cast(msg[count2])-1;

	while(!finish){

		if((node_id_2+1)!=-7){//Si encuentra un )
      //printf("Id1:%d\n",node_arr[node_id_1]->id);
      //printf("Id2:%d\n",node_arr[node_id_2]->id);
      //printf("Nodo1:%d\n",node_id_1);
      //printf("Nodo2:%d\n",node_id_2);
			add_child_v2(node_arr[node_id_1], node_arr[node_id_2]);//Crear hijo
			node_id_1=node_id_2;
			count2++;//Avanzar en el mensaje
			node_id_2=int_cast(msg[count2])-1;
		}else{
			count2++;//Avanzar en el mensaje
			node_id_2=int_cast(msg[count2])-1;//Avanzar en el mensaje
		}
		if((node_id_2+1)==-7){//Si encuentra un )
			node_id_1=(node_arr[node_id_1]->parent->id)-1;
		}
		if(count2==msg_lenght-1){
			finish=1;
		}

	}
}
//////Función para deserializar//////


void printNode(node_NT *level_node){
  printf("node:%d\n",level_node->id);
}

void remove_element(char *array, int index, int array_length)
{
   int i;
   for(i = index; i < array_length - 1; i++) array[i] = array[i + 1];
}
