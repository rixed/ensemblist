/*
Copyright (C) 2003 Cedric Cellier, Dominique Lavault

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include "slist.h"
#include "system.h"
#include "log.h"
#include "memspool.h"
#include "data.h"

struct MemoryStruct {
  char *memory;
  size_t size;
};

#define TAG_ENIGM_START "<enigm>"
#define TAG_ENIGM_CLOSE "</enigm>"
#define BASE_URL "http://rixed.free.fr/ensemblist/"
#define REMOTE_ENIGM_PATH_URL "uploads/"

static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	register int realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)data;

	mem->memory = (char *)gltv_memspool_realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory) {
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;
}

static void encode_url(char *url, size_t maxlen)
{
	unsigned i = 0;
	size_t len = strlen(url);
	while (url[i]!='\0') {
		if (url[i]==' ') {
			if (len>maxlen-2) gltv_log_fatal("encode_url: string too long");
			memmove(url+i+3, url+i+1, len-i);
			url[i] = '%';
			url[i+1] = '2';
			url[i+2] = '0';
			len += 2;
			i += 2;
		}
		i++;
	}
}

static void download(struct MemoryStruct * pChunk, const char * pUrl)
{
	CURL *curl_handle;
	char url[1024];
	strcpy(url, BASE_URL);
	strcat(url,pUrl);

	pChunk->memory=NULL; /* we expect realloc(NULL, size) to work */
	pChunk->size = 0;    /* no data at this point */

	/* init the curl session */
	curl_handle = curl_easy_init();
	if (curl_handle==NULL)
		return;	// ça chie dans le ventilo

	/* specify URL to get */
	encode_url(url, 1024);
	printf("getting URL %s\n", url);
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_FILE, (void *)pChunk);

	/* get it! */
	curl_easy_perform(curl_handle);

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);
}

static void processEnigmList(struct MemoryStruct * pChunk)
{
#define MAX_PATH 1024
	FILE * fpEnignList, * fpOut;
	char fileName[MAX_PATH], pathName[MAX_PATH];
	struct MemoryStruct enigmChunk;
	char * ptr = pChunk->memory,*  tmpPtr,* tmpFinPtr;
	enigmChunk.size=enigmChunk.memory=0;

	// 1.0 lister les fichiers présents online et qu'on a pas --------------------------
	// 1.0.1 rechercher le signet de début de fichier
	tmpPtr = strstr(ptr,TAG_ENIGM_START);
	if( tmpPtr == NULL)
		return;
	else
		tmpPtr+=strlen(TAG_ENIGM_START);

	// 1.0.2 rechercher le signet de fin de fichier
	tmpFinPtr= strstr(tmpPtr,TAG_ENIGM_CLOSE);
	if( tmpFinPtr == NULL) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "processEnigmList: no EOF sign");
		return;	// là, c'est moins normal
	}

	// 1.0.3 isoler le nom du fichier
	strncpy(fileName, tmpPtr, tmpFinPtr-tmpPtr);
	fileName[tmpFinPtr-tmpPtr]=0;	

	// 1.0.4 on l'a déjà?
	if ((fpEnignList=fopen(fileName,"r"))!=NULL)
		fclose(fpEnignList);	// on l'a
	else
	{
		// 1.1 downloader le fichier--------------------------
		// 1.1.2 get url
		strcpy(pathName,REMOTE_ENIGM_PATH_URL);
		strcat(pathName, fileName);
		download(&enigmChunk,pathName);
		// 1.1.3 save file
		if( (fpOut = fopen(fileName,"w"))!=NULL)
		{
			enigm *e;
			fwrite(enigmChunk.memory, 1, enigmChunk.size, fpOut);
			fclose(fpOut);
			// 1.2 charger l'énigme avec data_load_enigm, qui est reentrante.
			e = data_load_enigm(fileName);
			if (!e) gltv_log_fatal("processEnigmList: cannot load enigm");
			gltv_slist_insert(enigms, e);
			gltv_log_warning(GLTV_LOG_MUSTSEE, "get a new enigm from http : %s", e->name);
		}
		else
			gltv_log_warning(GLTV_LOG_MUSTSEE, "processEnigmList: unable to save enigm in %s : %s", fileName, strerror(errno));
	}
	ptr = tmpFinPtr;	// next, please

	if (enigmChunk.memory) gltv_memspool_unregister(enigmChunk.memory);
	return;
}

int with_net = 1;

void share()
{
	struct MemoryStruct chunk;
	if (!with_net) return;
	download(&chunk,"getdir.php");
	processEnigmList(&chunk);
	if (chunk.memory) gltv_memspool_unregister(chunk.memory);
}
