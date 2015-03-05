#include "pdfAnalyzer.h"


// This function checks if the header is correct
int checkMagicNumber(struct pdfDocument * pdf){
	
	
	int version_size = 8;
	//int ret;
	
	char * version;
	version = (char*)calloc(9,sizeof(char));
	version[8]='\0';
	
	
	// Go to the beginning of the file
	fseek(pdf->fh,0,SEEK_SET);
	
	
	// Read the 8 first bytes Ex: %PDF-1.
	fread(version,1,version_size,pdf->fh);
	
	//printf("pdf header = %s\n",version);
	
	if( strncmp(version,"%PDF-1.1",8) == 0 || strncmp(version,"%PDF-1.2",8) == 0 || strncmp(version,"%PDF-1.3",8) == 0 || strncmp(version,"%PDF-1.4",8) == 0 || strncmp(version,"%PDF-1.5",8) == 0 || strncmp(version,"%PDF-1.6",8) == 0 || strncmp(version,"%PDF-1.7",8) == 0 ){
	
		//printf ("PDF Header OK = %s\n",version);
		pdf->version = version;
	
	}else{
		//printf ("PDF Header KO : This document is not a PDF file.\n");
		pdf->testStruct->bad_header = 1;
		//pdf->version = "none";
		
		//TODO XDP files
		
		return -1;
	}
	
	return 0;
	
}

// This function get the content of the PDF document
int getPDFContent(struct pdfDocument * pdf){

	char * content = NULL;
	int doc_size = 0;
	int read_bytes;

	
	// Get the size in byte
	fseek(pdf->fh,0,SEEK_END);
	doc_size = ftell(pdf->fh);
	
	printf("Document Size  = %d\n",doc_size);
	
	content = (char*)calloc(doc_size+1,sizeof(char));
	content[doc_size]= '\0';

	
	fseek(pdf->fh,0,SEEK_SET);
	
	read_bytes = fread(content,1,doc_size,pdf->fh);
	
	//printf("read bytes = %d\n",read_bytes);
	
	//printf("Document content = %s",content);
	
	//read_bytes = strlen(content);
	//printf("read bytes = %d\n",read_bytes);
	
	pdf->content = content;
	
	pdf->size = read_bytes;
	
	return read_bytes;
	
}



// This function decode a dictionnary obfuscated with hexa; return the modified dico or the original if there is no obfuscation.
char * hexaObfuscationDecode(char * dico){

	char * start = NULL;
	int len = 0;
	char * decoded_dico = NULL;
	char * hexa = NULL;
	char * hexa_decoded = NULL;
	int is_space_hexa = 1;
	char * tmp = NULL;

	len = strlen(dico);
	start = searchPattern(dico,"#",1,len);


	if(start == NULL){
		return NULL;
	}

	//printf("dico = %s\n\n",dico);

	//decoded_dico = (char*)calloc(len,sizeof(char));
	hexa = (char*)calloc(4,sizeof(char));
	hexa[3] = '\0';

	//hexa = (char*)calloc(2,sizeof(char));
	hexa_decoded = (char*)calloc(2,sizeof(char));
	hexa_decoded[1] = '\0';



	//memcpy(decoded_dico,dico,len);
	tmp = (char*)calloc(len+1,sizeof(char));
	tmp[len]= '\0';
	memcpy(tmp,dico,len);



	while( start != NULL && len >= 3){

		

		// get the pointer of the hexa code
		start = getHexa(start,len);

		
		if(start == NULL){
			//start += 3;
			//len -=3;
			//len = 0;
			continue;
		}
			
		
		memcpy(hexa, start, 3);

		
		if(strncmp(hexa,"#20",3) != 0){
			is_space_hexa = 0;
		}

		//memcpy(hexa, start, 2);
		//hexa[2]='\0';
		//printf("hexa = %s\n",hexa);

		//sscanf(hexa,"%x",&hexa_decoded[0]);
		sscanf(hexa+1,"%x",&hexa_decoded[0]);

		//printf("hexa_decoded_s = %s\n",hexa_decoded);

		decoded_dico = replaceInString(tmp,hexa,hexa_decoded);

		if(decoded_dico != NULL){

			free(tmp);
			tmp = NULL;

			tmp = decoded_dico;

			//printf("decoded_dico___  = %s\n\n",tmp);


		}else{
			printf("Error :: hexaObfuscationDecode :: replaceInString returns NULL \n");
			return NULL;
		}


		start += 3;
		//start += 3;

		len = (int)(start - dico);
		len = strlen(dico) -len;
		//printf("len = %d\n",len);


	}

	free(hexa);
	free(hexa_decoded);

	if(decoded_dico != NULL && is_space_hexa == 0){
		//printf("decoded_dico  = %s\n\n",decoded_dico);
		return decoded_dico;
	}
		


	return NULL ; 

}

//This function get the object dictionary
char * getObjectDictionary(struct pdfObject * obj, struct pdfDocument * pdf){
	
	char  * dico = NULL;
	char * decoded_dico = NULL;
	char * content =  obj->content;
	char * dico_start = NULL;
	//char * dico_end = NULL;
	int len = 0;
		
	//char* src, char* pat , int pat_size ,  int size


	
	// Search the beginning of the
	dico_start = searchPattern(content,"<<",2,obj->content_size);


	
	if(dico_start == NULL){
		//printf("No dictionary found in object %s!!\n", obj->reference);
		return NULL;
	}

	//printf("hey == %s\n",content);
	
	// TODO search other occurencies of "<<" to detect sub dictionaries
	// TODO Found the same number of occurencies of stream ">>"
	
	len = (int)(dico_start - obj->content);
	len = obj->content_size - len;

	//printf("getDico :: len = %d\n",len);

	dico =  getDelimitedStringContent(dico_start,"<<", ">>", len);


	if(dico == NULL){
		printf("Warning :: getObjectDictionary :: No dictionary found in object %s\n",obj->reference);
		return NULL;
	}

	len = strlen(dico);

	//printf("hey\n");

	//printf("dico = %s---->\n",dico);
	

	// decode hexa obfuscated dictionaries
	//decoded_dico = hexaObfuscationDecode(dico);

	if(decoded_dico != NULL){
		printf("Warning :: getObjectDictionary :: Obfuscated Object dictionary in %s\n", obj->reference);
		free(dico);
		pdf->testStruct->obfuscated_object ++;
		return decoded_dico;
	}

	free(decoded_dico);
	return dico;
	
}


//This function get the object dictionary
char * getObjectDictionary_old(struct pdfObject * obj){
	
	char  * dico = NULL;
	char * content =  obj->content;
	char * dico_start = NULL;
	char * dico_end = NULL;
	int len = 0;	
	
	//char* src, char* pat , int pat_size ,  int size
	
	// Search the beginning of the
	dico_start = searchPattern(content,"<<",2,obj->content_size);
	
	if(dico_start == NULL){
		//printf("No dictionary found in object %s!!\n", obj->reference);
		return NULL;
	}
	
	// TODO search other occurencies of "<<" to detect sub dictionaries
	// TODO Found the same number of occurencies of stream ">>"
	
	len = dico_start - obj->content;	
	
	dico_end = searchPattern(dico_start,">>",2,obj->content_size-len);
	
	if(dico_end == NULL){
		//printf("No dictionary found in object %s!!\n", obj->reference);
		return NULL;
	}
	
	len = dico_end - dico_start + 2; // +2 is for >>
	
	//dico = (char*)malloc(len*sizeof(char));
	dico = (char*)calloc(len,sizeof(char));
	
	strncpy(dico,dico_start,len);
	
	// store the dictionary length
	//obj->dico_len = len;
	
	//printf("dictionary  = %s\n",dico);
	
	
	
	return dico;
	
}




char * getObjectType(struct pdfObject * obj){

	char * type = NULL;
	char * start = NULL;
	char * end = NULL;
	int len = 0;
	int flag  = 0;
	char * tmp = NULL;

	//char tmp = 0;
	//char *

	// Fix bug => /Subtype /Type1 /Type/Font
	tmp = obj->dico;
	while(flag == 0){

		start = searchPattern(tmp,"/Type",5,strlen(obj->dico));

		if(start == NULL ){
			//printf("I got no type !!\n");
			return NULL;
		}


		if( start[5] == '1' || start[5] == '2'){ // /Type2 /Type1
			tmp = start+1;

		}else{
			flag ++;
		}
		//start = searchPattern(obj->dico,"/Type",5,strlen(obj->dico));
	}
	
	//start = searchPattern(obj->dico,"/Type",5,strlen(obj->dico));

	
	
	if(start == NULL ){
		//printf("I got no type !!\n");
		return NULL;
	}
	
	len = (int)(start - obj->dico);
	//printf("len2 = %d :: %d :: %d\n",len, start, obj->dico);
	
	start += 5;

	// White space
	if(start[0] == ' '){
		start ++;
	}
	//printf("len = %d \t start = %d \t obj->dico = %d\n",len, start,obj->dico);
	
	end = memchr(start,'/',strlen(obj->dico)-len);
	
	
	if(end == NULL){
		//printf("Err :: end = NULL\n");
		return NULL;
	}
	
	len = 0;
	do{
		//printf("char = %c\n",end[0]);
		end ++;
		len++;
		
	}while( (end[0] >=97 && end[0] <=122) || (end[0] >=65 && end[0] <=90 ) || (end[0] >=48 && end[0] <= 57) ); // Lower case [97 122] or Upper case [65 90] + digit [48 57]
	
	type = (char*)calloc(len+1,sizeof(char));
	type[len]= '\0';
	//printf("getObjectType :: len = %d \n",len);
	//start += 4;
	len = (int)(end-start);
	strncpy(type,start,len);
	
	
	return type;
}


// This function get the stream content of an object (returns NULL if there is no stream)
char * getObjectStream(struct pdfObject * obj){

	char * stream = NULL;
	char * start = NULL;
	char * end = NULL;
	int len = 0;
	//char * tmp = NULL;


	start = searchPattern(obj->content,"stream",6,obj->content_size);

	
	if(start == NULL){
		return NULL;
	}

	
	len = (int)(start - obj->content);
	len = obj->content_size -len;

	//printf("getObjectStream :: len = %d\n",len);

	//printf("start = %d\n",start);
	end = searchPattern(start,"endstream",9,len);
	//printf("end = %d\n",end);

	if(end == NULL){
		return NULL;
	}
	
	//len = (int)(end-start);
	
	// Remove white space after "stream" and before "endstream"
	start += 6;
	while(start[0] == '\n' || start[0] == '\r'){
		start ++;
	}
	
	end --;
	while(end[0] == '\n' || end[0] == '\r'){
		end --;
	}
	end++;
	//printf("end = %d\n",end);

	len = (int)(end-start);
	if(len <= 0 ){
		printf("Warning :: Empty stream content in object %s\n", obj->reference);
		return NULL;
	}


	//printf("Stream len = %d\n",len);

	obj->stream_size = len; // -1 for the white space
	obj->tmp_stream_size = obj->stream_size;

	stream = (char*)calloc(len+1,sizeof(char));
	stream[len]='\0';
	
	memcpy(stream,start,len);

	return stream;

}



char * getStreamFilters(struct pdfObject * obj){

	char * start = NULL;
	char * end = NULL;
	char * end_tmp = NULL;
	int len = 0;
	char * filters = NULL;
	


	
	start = searchPattern(obj->dico,"/Filter",7,strlen(obj->dico));
	//printf("getStreamFilters :: start = %d\n",start);
	
	if( start == NULL ){
		//printf("I got no filter !!\n");
		return NULL;
	}
	
	len = (int)(start - obj->dico);
	start += 1;
	//printf("len = %d \t start = %d \t obj->dico = %d\n",len, start,obj->dico);
	
	
	
	end = memchr(start,'/',strlen(obj->dico)-len);
	end_tmp = memchr(start,'[',strlen(obj->dico)-len);
	
	if(end == NULL){
		return NULL;
	}

	
	// if a bracket is before the first / that means it's an array of filters
	if( end_tmp != NULL && end_tmp < end ){

		//printf("getStreamFilters :: heyhey = %d\n",end);
		
		end  = end_tmp;
		start = end;
		
		len = 0;
		do{
			//printf("char = %c\n",end[0]);
			// get the array of filters
			end ++;
			len++;
			//printf("heyhey\n");
		
		}while( end[0] != ']');
		//}while( (end[0] >=97 && end[0] <=122) || (end[0] >=65 && end[0] <=90 ) || (end[0] >=48 && end[0] <= 57) || (end[0] == ' ') || (end[0] == '/') ); // Lower case [97 122] or Upper case [65 90] + digit [48 57]
		len +=2;
		//printf("getStreamFilters :: len = %d\n",len);
		//printf("end debug :: %c",end[0]);


	}else{ // a single filter
	
		start = end;
		len = 0;
		do{
			//printf("char = %c\n",end[0]);
			end ++;
			len++;
		
		}while( (end[0] >=97 && end[0] <=122) || (end[0] >=65 && end[0] <=90 ) || (end[0] >=48 && end[0] <= 57) ); // Lower case [97 122] or Upper case [65 90] + digit [48 57]
		
	
	}
	
	
	//filters = (char*)malloc(len*sizeof(char));
	filters = (char*)calloc(len+1,sizeof(char));
	filters[len] = '\0';
	//printf("len = %d \n",len);

	//start += 6;
	//len = (int)(end - start);
	//printf("getStreamFilters :: len = %d \n",len);

	strncpy(filters,start,len);
	
	//printf("filters = %s \n",filters);
	
	
	return filters;

}



// This function decode an object stream according to the filters applied
int decodeObjectStream(struct pdfObject * obj){

	
	char * start = NULL;
	char * end = NULL;
	char * filter =NULL; 
	char * stream = NULL;
	int len = 0;
	int i =0;
	int filter_applied = 0;
	
	
	
	if(obj->filters == NULL){
		printf("Error :: There is no filter implemented\n");
		return -1;
	}
	
	//printf("implemented filters = %s\n",obj->filters);
	if(obj->stream == NULL){
		printf("Error :: Stream NULL\n");
		return -1;
	}

	stream = obj->stream;
	end = obj->filters;




	//while( (start = memchr(end, '/',strlen(obj->filters)-len)) != NULL ){
	while( (start = strchr(end, '/')) != NULL ){

		//printf("Searching filter in :: %d :: %s\n",end,end);
		//printf("Searching filter in :: %d :: %s\n",start,start);
		
		//filter ++;
		i = 0;
		end = start;		
		// Scan the filter name
		do{
			end ++;
			i++;
		}while( i< strlen(obj->filters) && ((end[0] >=97 && end[0] <=122) || (end[0] >=65 && end[0] <=90 ) || (end[0] >=48 && end[0] <= 57)));
		
		len = (int)(end-start);
		
		//filter = (char*)malloc(len*sizeof(char));
		filter = (char*)calloc(len+1,sizeof(char));
		filter[len] = '\0';

		//printf("filter_len = %d\n",len);

		strncpy(filter,start,len);
		//printf("implemented filter_end = %s\n",filter);
		
		//len -= filter - strlen(obj->filter);
		
		
		// Apply decode filter
		
		//TODO
		if((strncmp(filter,"/FlateDecode",12) == 0 && strncmp(filter,"/FlateDecode",strlen(filter)) == 0) || (strncmp(filter,"/Fl",3) == 0 && strncmp(filter,"/Fl",strlen(filter)) == 0)){
				printf("Decode Fladetecode \n");
				stream = FlateDecode(stream, obj);
				/*if(strncmp(obj->reference,"8 0 obj",8) == 0)
					printf("stream _flatedecode = %s\n",stream);*/
				filter_applied ++;
		}else{

			if((strncmp(filter,"/ASCIIHexDecode",15) == 0 && strncmp(filter,"/ASCIIHexDecode",strlen(filter)) == 0) || (strncmp(filter,"/AHx",4) == 0 && strncmp(filter,"/AHx",strlen(filter)) == 0)){
				//printf("Decode ASCIIHexDecode \n");
				stream = ASCIIHexDecode(stream, obj);
				filter_applied ++;
		
			}else{

				
				if((strncmp(filter,"/ASCII85Decode",14) == 0 && strncmp(filter,"/ASCII85Decode",strlen(filter)) == 0) || (strncmp(filter,"/A85",4) == 0 && strncmp(filter,"/A85",strlen(filter)) == 0)){
					//if(strncmp(obj->reference,"44 0 obj",8) == 0)
					//printf("Decode ASCII85Decode \n");
					stream = ASCII85Decode(stream, obj);
					filter_applied ++;
				}else{

					
					if((strncmp(filter,"/LZWDecode",10) == 0 && strncmp(filter,"/LZWDecode",strlen(filter)) == 0) || (strncmp(filter,"/LZW",4) == 0 && strncmp(filter,"/LZW",strlen(filter)) == 0)){
						//printf("Decode LZWDecode \n");
						stream = LZWDecode(stream,obj);
						filter_applied ++;
					}else{

						//TODO
						if((strncmp(filter,"/RunLengthDecode",16) == 0 && strncmp(filter,"/RunLengthDecode",strlen(filter)) == 0) || (strncmp(filter,"/RL",3) == 0 && strncmp(filter,"/RL",strlen(filter)) == 0) ){
							printf("Warning :: Filter RunLengthDecode not implemented :: %s\n",obj->reference );
							filter_applied = 0;
										
						}else{

							

							if((strncmp(filter,"/CCITTFaxDecode",15) == 0 && strncmp(filter,"/CCITTFaxDecode",strlen(filter)) == 0) || (strncmp(filter,"/CCF",4) == 0 && strncmp(filter,"/CCF",strlen(filter)) == 0)){
								//if(strncmp(obj->reference,"11 0 obj",8) == 0)
								//printf("Decode CCITTFaxDecode \n");
								stream = CCITTFaxDecode(stream,obj);

								filter_applied ++;
								
							}else{

								printf("Filter %s  in object %s not implemented\n",filter,obj->reference);

							}

						}


					}

				}


			}

		}
		
		
		
	}
	
	
	// Store the decoded stream
	if(stream != NULL && filter_applied > 0){
		obj->decoded_stream = stream ;
	}

	free(filter);
	
	
	return 0;
}



// Get object information (types, filters, streams, etc.)
int getObjectInfos(struct pdfObject * obj, struct pdfDocument * pdf){

	char * dico = NULL;
	char * type = NULL;
	char * stream = NULL;
	char * filters = NULL;
	//int res = 0;
	

	if(obj == NULL){
		printf("Error :: getObjectInfos :: null object\n");
		return -1;
	}
	
	

	// Get the dictionary
	dico = getObjectDictionary(obj,pdf);
	if(dico == NULL){
		return 0;
	}
	//printf("dictionary = %s\n\n",dico);
	obj->dico = dico;
	
	
	
	// Get the type
	type = getObjectType(obj);
	if(type != NULL){
		//printf("/Type = %s\n\n",type);
		obj->type = type;
	}

	
	
	

	// Get stream content
	stream  = getObjectStream(obj);

	if(stream != NULL){
		//printf("stream = %s--\n\n",stream);
		obj->stream = stream;
	}

	// Get stream filters
	filters = getStreamFilters(obj);
	
	if(filters != NULL){
		obj->filters = filters;
		decodeObjectStream(obj); // to uncomment
		//printf("res = %d\n",res);
	}

	// debug
	
	/*if(strncmp(obj->reference,"19 0 obj",sizeof(obj->reference)) == 0){
		printObject(obj);
	}*/
	
		
	return 1;
	

}



// This function extract object in an object stream and add them in the object list
int extractObjectFromObjStream(struct pdfDocument * pdf, struct pdfObject *obj){


	int first = 0;
	int num = 0;
	//char * num_a = NULL;
	//char * first_a = NULL;
	char * stream  = NULL;
	char * start = NULL;
	char * end = NULL;
	int len = 0;
	int i = 0;
	//int obj_num = 0;
	char * obj_num_a = NULL;
	int off = 0;
	//char * off_next = 0;
	//char * obj_num_next_a = 0;
	char * off_a = NULL;
	char * obj_ref = NULL;
	int obj_ref_len = 0;
	char * obj_content = NULL;
	int * obj_offsets;
	int obj_len = 0;
	struct pdfObject * comp_obj  =NULL;
	int stream_len = 0;




	// verif params
	if( pdf == NULL || obj == NULL ){
		printf("Error :: NULL Params\n");
		return -1;
	}

	if( obj->decoded_stream != NULL ){

		stream = obj->decoded_stream;
		stream_len = obj->decoded_stream_size;

	}else{
		stream = obj->stream;
		stream_len = obj->stream_size;
	}

	if(stream == NULL){
		printf("Error :: No stream in the Object stream %s\n",obj->reference);
		return -1;
	}

	if( obj->dico == NULL){
		printf("Error :: No dictionary in the Object stream %s\n",obj->reference);
		return -1;
	}


	printf("::: extractObjectFromObjStream :::\n");
	//printf("stream = %s\n\n",stream);
	//printf("strm len = %d\n",strlen(obj->dico));

	/*
	if(strncmp(obj->reference,"15 0 obj",8) == 0){
			printf("stream = %s\n\n",stream);
	}
	*/

	// Get the number of object embedded in the stream => N in the dictionary
	start = searchPattern(obj->dico,"/N",2,strlen(obj->dico));
	//printf("start = %d\n",start);	

	if( start == NULL){
		printf("Error :: Entry /N not found in Object stream dictionary %s\n",obj->reference);
		return -1;
	}

	
	end = start;
	//printf("end[0] = %c\n",end[0]);
	start +=2;


	// if there is a space after /N
	if(start[0] == ' '){
		start ++ ;
	}

	end = start;

	len = strlen(obj->dico) - (int)(start - obj->dico);
	num = getNumber(start,len);
	//printf("num = %d\n",num);

	/*len = 0;
	do{
		//printf("end[0] = %c\n",end[0]);
		len ++;
		end ++;
	}while(end[0] >= 48 && end[0] <= 57);

	//printf("len = %d\n",len);

	num_a = (char*)calloc(len,sizeof(char));
	memcpy(num_a,start,len);

	num = atoi(num_a);
	printf("num = %d\n",num);
	*/


	if(num <= 0){
		printf("Error:: Incorrect /N entry in object stream %s\n",obj->reference);
		return -1;
	}


	// Get the byte offset of the first compressed object "/First" entry in dico
	start = searchPattern(obj->dico,"/First",6,strlen(obj->dico));
	//printf("start = %d\n",start);	

	if( start == NULL){
		printf("Error :: Entry /First not found in Object stream dictionary %s\n",obj->reference);
		return -1;
	}

	
	end = start;
	//printf("end[0] = %c\n",end[0]);
	start +=6;


	// if there is a space after /First (TODO Replace with a while to prevent several white spaces)
	if(start[0] == ' '){
		start ++ ;
	}

	end = start;
	len = 0;
	len = strlen(obj->dico) - (int)(start - obj->dico);
	first = getNumber(start,len);
	//printf("first = %d\n",first);


	if(first <= 0){
		printf("Error:: Incorrect /First entry in object stream %s\n",obj->reference);
		return -1;
	}


	
	start = stream;
	//printf("start[0] = %c\n",start[0]);


	//len = strlen(stream);
	len = stream_len;

	obj_offsets = (int*)calloc(num,sizeof(int));

	//printf("len_obj_stream = %d\n",len);
	

	// Get objects number and offset
	for(i = 0 ; i< num; i++){

		//printf("hey!!\n");
		//printf("start[0] = %c\n",start[0]);
		// Get the object number
		obj_num_a = getNumber_a(start,len);

		//printf("Hey :: %d :: %d\n",start,len);

		len -=  strlen(obj_num_a);
		start += strlen(obj_num_a);

		//printf("obj_num = %s\n",obj_num_a);


		// Move ptr for white space
		start ++ ;

		// Get the offset
		off_a = getNumber_a(start,len);
		off = atoi(off_a);

		obj_offsets[i] = off;


		len -=  strlen(off_a);
		start += strlen(off_a);

		//printf("off = %s\n",off_a);

		// Move ptr for white space
		start ++ ;
		//printf("start[0] = %c\n",start[0]);


		// calc the length of the object according to the offset of the next object.		
	}

	printf("\n\n");

	start = stream;
	len = strlen(stream);

	// // Get objects content
	for(i = 0 ; i< num; i++){


		// init object
		comp_obj = initPDFObject();

		if(comp_obj == NULL){
			printf("PDF object initilizing failed\n");
			return -1;
		}

		// Get the object number
		obj_num_a = getNumber_a(start,len);





		// "X O obj"
		// Build the object reference
		obj_ref_len = strlen(obj_num_a) + 6;
		obj_ref = (char*)calloc(obj_ref_len+1,sizeof(char));
		obj_ref[obj_ref_len] = '\0';

		obj_ref = strncat(obj_ref, obj_num_a, strlen(obj_num_a));
		obj_ref = strncat(obj_ref, " 0 obj", 6);
		//printf("obj_ref = %s\n",obj_ref);
		comp_obj->reference =  obj_ref;


		// move ptr according to the size of the scanned number.
		len -=  strlen(obj_num_a);
		start += strlen(obj_num_a);


		//printf("obj_num = %s\n",obj_num_a);


		// Move ptr for white space
		start ++ ;

		// Get the offset
		off_a = getNumber_a(start,len);
		off = atoi(off_a);

		len -=  strlen(off_a);
		start += strlen(off_a);
		//printf("off = %s\n",off_a);

		// Move ptr for white space
		start ++ ;


		// offset in stream = off + first
		// real offset = start + off + first.

		// calc the length of the object according to the offset of the next object.
		if( i != num-1 ){
			obj_len  = ( obj_offsets[i+1] - off);
			//printf("obj_len = %d\n",obj_len);
		}else{
			// calc according to the end of the stream
			//obj_len =  strlen(stream) - ( (stream + first + off) - stream ) ;
			obj_len =  stream_len - ( (stream + first + off) - stream ) ;
			//printf("obj_len = %d\n",obj_len);
		}
		
		obj_content = (char*)calloc(obj_len+1,sizeof(char));

		
		obj_content[obj_len] = '\0';

		// offset of the object content
		end = stream + first + off;

		
		// Get content
		memcpy(obj_content,end,obj_len);
		

		//printf("extractObjectFromObjStream :: obj_content = %s\n", obj_content);


		comp_obj->content = obj_content;
		comp_obj->content_size = obj_len;

		//  Get object informations
		getObjectInfos(comp_obj,pdf);



		addObjectInList(comp_obj,pdf);

		
	}


	return 0;
}





// This function get all objects defined in the document
int getPDFObjects(struct pdfDocument * pdf){

	char * startobj_ptr;
	char * endobj_ptr;
	char * content;
	int len = 0;
	char * ref = NULL;
	int gen_num_len = 0;
	int obj_num_len = 0;
	int ref_len =0;
	int tmp = 0;
	struct pdfObject* obj = NULL;
	int offset = 0;
		
	
	//startobj_ptr = startobj_ptr - pdf->content;
	//printf("content ptr = %d\n\n",pdf->content);
	//printf("content ptr = %s\n",pdf->content);
	
	//startobj_ptr = (char*)malloc(gen_num_len*sizeof(char));
	
	endobj_ptr = pdf->content;
	//tmp = pdf->size;
	//len = pdf->size;


	
	
	while( (startobj_ptr = strstr(endobj_ptr,"obj")) != NULL){
	//while( (startobj_ptr = searchPattern(endobj_ptr,"obj",3,len) ) != NULL) {		
	
		//printf("search offset = %d\n",endobj_ptr);
		gen_num_len = 0;
		obj_num_len = 0;
		
		startobj_ptr -= 2; // point to the generation number
		//printf("Generation number = %c\n",startobj_ptr[0]);

		// Check the generation number pointer
		if(startobj_ptr[0] < 48 || startobj_ptr[0] > 57){
			//printf("This is not a generation number:: %c \n",startobj_ptr[0]);
			endobj_ptr = startobj_ptr+3;
			continue;
		}



		while(startobj_ptr[0] >= 48 && startobj_ptr[0] <= 57  ){
		
			//printf("Warning :: Treat this case 1 :: \n");
			startobj_ptr--;
			//printf("Warning :: Treat this case 1 :: %c\n", startobj_ptr[0] );
			gen_num_len++;
		}


	
		startobj_ptr -= 1; // point to the object number

		// Check the object number pointer
		if(startobj_ptr[0] < 48 || startobj_ptr[0] > 57){
			//printf("This is not a generation number:: %c \n",startobj_ptr[0]);
			endobj_ptr = startobj_ptr + gen_num_len + 4;
			continue;
		}

		//printf("object number = %c\n",startobj_ptr[0]);
		while(startobj_ptr[0] >= 48 && startobj_ptr[0] <= 57  ){
		
			startobj_ptr--;
			//printf("Warning :: Treat this case 2 :: %c\n",startobj_ptr[0] );
			obj_num_len ++;
			
		}
		
		startobj_ptr++;
		
		ref_len = gen_num_len + 1 + obj_num_len + 1 + 3 ; // 1 = space and 3 = obj


		ref = (char*)calloc(ref_len+1,sizeof(char));
		ref[ref_len] = '\0';
		
		strncpy(ref,startobj_ptr,ref_len);
		printf("object reference = %s :: %d\n",ref,ref_len);
		
		//startobj_ptr++;
		
		offset = (int)(startobj_ptr -pdf->content);
		
	
		//printf("object offset = %d\n",offset);

		//printf("start obj ptr = %d\n",startobj_ptr);
		
		//endobj_ptr = strstr(startobj_ptr,"endobj");
		//printf("end obj ptr 1 = %d\n",endobj_ptr);
		tmp = (int)(pdf->size - (startobj_ptr - pdf->content));
		//printf("size of block = %d\n",tmp);
		endobj_ptr = searchPattern(startobj_ptr,"endobj",6,tmp);


		//printf("end obj ptr = %d\n",endobj_ptr);
		
		if(endobj_ptr == NULL){
			//printf(":: Error :: startobj_ptr = %d\n", startobj_ptr);
			return -1;
		}
		//printf("end obj ptr = %c\n",endobj_ptr[0]);
		
		
		endobj_ptr += 6;
		//printf("end obj ptr = %c\n",endobj_ptr[0]);
		
	
		len = (int)(endobj_ptr - startobj_ptr);
		//printf("object len = %d\n",len);
	
		//content = (char*)malloc(len*sizeof(char));
		content = (char*)calloc(len+1,sizeof(char));
		content[len]='\0';
		
		memcpy (content, startobj_ptr,len);
		
		// Create a object
		
		//printf("\nobj content --\n%s--\n\n\n",content);
		
		obj = initPDFObject();		
		
		//printf("obj_content size = %d\n",len);
		//printf("obj_content = %s\n",content);
		if( obj != NULL){
			obj->reference = ref;
			obj->content = content;
			obj->offset = offset;
			obj->content_size = len;
			
			getObjectInfos(obj,pdf);
			
		}


		// Extract object emebedded in object stream
		if(obj->type != NULL && strncmp(obj->type,"/ObjStm",7) == 0 ){
			extractObjectFromObjStream(pdf,obj);
		}
		
		
		
		// Add in object list.
		addObjectInList(obj,pdf);
		printf("------------------------------------\n\n");


	
	
	}
	
	//printf("content ptr = %d\n",pdf->content);
	//printf("startobj ptr = %s\n",startobj_ptr);
	
	
	return 0;
}


// Get pdf trailer according to PDF
int getPDFTrailers_1(struct pdfDocument * pdf){

	char * content = NULL;
	char * decoded_content = NULL;
	char * encrypt = NULL;
	char * start = NULL; 
	char * end = NULL;
	int len = 0;
	struct pdfTrailer * trailer;


	end = pdf->content;
	len = pdf->size;

	while( (start = searchPattern(end,"trailer",7,len)) ){

		//printf("hoo\n");

		len = (int)(start - end);
		len = pdf->size -len ;
		end = searchPattern(start,"%%EOF",5,len);
		end += 5;

		len = (int)(end - start);
		content = (char*)calloc(len+1,sizeof(char));
		content[len] = '\0';

		memcpy(content,start,len);
	
		if(!(trailer = initPDFTrailer())){
			printf("Error :: while initilaizing pdfTrailer structure\n");
			return -1;
		}
		
		//printf("hoo :: %s\n",content);
		// check is the trailer dictionary is no hexa obfuscated
		decoded_content = hexaObfuscationDecode(content);
		//printf("hoo\n");


		if(decoded_content != NULL){
			printf("Warning :: getPDFTrailers_1 :: Obfuscated trailer dictionary !!\n");
			pdf->testStruct->obfuscated_object ++ ;
			trailer->content = decoded_content;
		}else{
			trailer->content = content;	
		}

		// check if the file is encrypted
		if( (encrypt = searchPattern(trailer->content,"/Encrypt",8,len)) != NULL){
			printf("Warning :: getPDFTrailers_1 :: This PDF Document is encrypted !\n");
			pdf->testStruct->encrypted = 1;
		}

		addTrailerInList(pdf,trailer);

		//pdf->trailers = trailer;
		//printf("trailer content = %s\n",trailer->content);

		len = (int)( end - pdf->content);
		len = pdf->size - len;

		//printf("\n");

	}

	return 0;
	
}


// Get pdf trailer according to PDF version starting from 1.5
int getPDFTrailers_2(struct pdfDocument * pdf){

	char * content = NULL;
	char * start = NULL; 
	char * end = NULL;
	int len = 0;
	struct pdfTrailer * trailer;


	end = pdf->content;
	len = pdf->size;

	while( (start = searchPattern(end,"startxref",9,len) ) != NULL ){

		len = (int)(start - end);
		len = pdf->size -len ;
		end = searchPattern(start,"%%EOF",5,len);
		end += 5;
		

		len = (int)(end - start);
		content = (char*)calloc(len+1,sizeof(char));
		content[len] = '\0';

		memcpy(content,start,len);
	
		if(!(trailer = initPDFTrailer())){
			printf("Error :: while initilaizing pdfTrailer structure\n");
			return -1;
		}
		
		// TODO improve
		trailer->content = content;

		addTrailerInList(pdf,trailer);

		//pdf->trailers = trailer;
		//printf("trailer content = %s\n",content);

		len = (int)( end - pdf->content);
		len = pdf->size - len ;

		//printf("\n\n");

	}

	/*
	
	start = searchPattern(pdf->content,"startxref",9,pdf->size);
	
	if(start == NULL ){
		return -1;
	}
	
	len = (int)(start - pdf->content);
	//len = obj->content_size - len;
	
	end = searchPattern(start,"%%EOF",5,pdf->size-len);
	
	
	if(end == NULL){
		return -1;
	}
	
	len = (int)(end - start);
	
	content = (char*)calloc(len,sizeof(char));
	
	memcpy(content,start,len);
	
	if(!(trailer = initPDFTrailer())){
		printf("Error :: while initilaizing pdfTrailer structure\n");
		return -1;
	}
	
	trailer->content = content;
	pdf->trailers = trailer;

	
	printf("trailer content = %s\n",content);
	*/
	
	// TODO get several trailers

	return 0;
	
}


// This function remove all PostScript comments in the pdf document
int removeComments(struct pdfDocument * pdf){

/*
	char * content = NULL;
	char * whithout_comments = NULL;
	char * comments = NULL;
	char * start = NULL;
	char * end = NULL;
	int len = 0;

	len = pdf->size;
	

	while(len > 0){

		

		len --;
	}


*/


	return 0;
}


int parsePDF(struct pdfDocument * pdf){

	if(pdf == NULL){
		return -1;
	}

	printf("\n");
	printf("------------------------------\n");
	printf("---  PDF DOCUMENT PARSING  ---\n");
	printf("------------------------------\n\n");


	// Get the content of the document
	getPDFContent(pdf);

	// Remove comments
	//removeComments(pdf);

	// Get Trailers
	
	
	getPDFTrailers_1(pdf);
	if(pdf->trailers == NULL){
		getPDFTrailers_2(pdf);

		if(pdf->trailers == NULL)
			pdf->testStruct->bad_trailer ++;
	}


	// if the document is encrypted
	if( pdf->testStruct->encrypted > 0 ){		
		return -2;
	}

	// Get objects described in pdf document
	getPDFObjects(pdf);
	

	return 0;

}

