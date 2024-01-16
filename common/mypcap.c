#include "mypcap.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>


pcap_file_t *mypcap_open(const char* filename) 
{
	    pcap_file_t *pcap = safe_malloc(sizeof(pcap_file_t));
	        size_t nbread;
		    pcap->fp = open(filename, O_RDONLY);
		        if (pcap->fp < 0) {
				        fprintf(stderr, "open %s failed\n", filename);
					        return NULL;
						    }

			    nbread = read(pcap->fp, &pcap->hdr, sizeof(pcap_hdr_t));
			        if (nbread != sizeof(pcap_hdr_t)) {
					        fprintf(stderr, "Error reading pcap header\n");
						        return NULL;
							    }

				    /* pcap processed by 'tshark' has leading bytes */
				    if (pcap->hdr.magic_number != PCAP_MAGIC_NANO && pcap->hdr.magic_number != PCAP_MAGIC_USEC) {
					            size_t offset = ((uint32_t *)(&pcap->hdr))[1];
						           
						            nbread = lseek(pcap->fp, offset+16+4, SEEK_CUR);
							        }

				        return pcap;
}

pcap_file_t *mypcap_open_memory(const char *data, size_t len) 
{
	    pcap_file_t *pcap = safe_malloc(sizeof(pcap_file_t));
	        
	        memcpy(&pcap->hdr, data, sizeof(pcap_hdr_t));
		    size_t offset = 0;
		        if (pcap->hdr.magic_number != PCAP_MAGIC_NANO && pcap->hdr.magic_number != PCAP_MAGIC_USEC) {
				        offset = ((uint32_t *)(&pcap->hdr))[1] + 8;
					    }

			    pcap->data_len = len - sizeof(pcap_hdr_t) - offset;
			        pcap->data_ptr = (char *)data + sizeof(pcap_hdr_t) + offset;
				    return pcap;
}

void mypcap_close(pcap_file_t *pcap) 
{
	   close(pcap->fp);
	       safe_free(pcap);
}

char* mypcap_next(pcap_file_t *pcap, pcaprec_hdr_t *header) 
{
	    size_t nbread = read(pcap->fp, header, sizeof(pcaprec_hdr_t));

	        if (!nbread) {
			        return NULL;
				    }
		    else if(nbread != sizeof(pcaprec_hdr_t)) {
			            fprintf(stderr, "Error reading pcap header\n");
				            return NULL;
					        }

		        nbread = read(pcap->fp, pcap->buffer, header->incl_len);
			    if (nbread != header->incl_len) {
				            fprintf(stderr, "Error reading pcap packet\n");
					            return NULL;
						        }
			        return pcap->buffer;
}

int mypcap_readnext(pcap_file_t *pcap, pcaprec_hdr_t *header, char* buffer) 
{
	    size_t nbread = read(pcap->fp, header, sizeof(pcaprec_hdr_t));

	        if (!nbread) {
			        return 1;
				    }
		    else if(nbread != sizeof(pcaprec_hdr_t)) {
			            fprintf(stderr, "Error reading pcap header\n");
				            return -1;
					        }

		        nbread = read(pcap->fp, buffer, header->incl_len);
			    if (!nbread) {
			        return 1;
				    }
		    	else if (nbread != header->incl_len) {
				            fprintf(stderr, "Error reading pcap packet\n");
					            return -1;
						        }
			        return 0;
}


char* mypcap_next_memory(pcap_file_t *pcap, pcaprec_hdr_t *header) 
{
	    if (pcap->data_len < sizeof(pcaprec_hdr_t)) {
		            return NULL;
			        }

	        if (pcap->data_ptr == NULL) {
			        fprintf(stderr, "pcap->data_ptr is NULL\n");
				        return NULL;
					    }

			memcpy((char *)header, pcap->data_ptr, sizeof(pcaprec_hdr_t));
		     
		        pcap->data_ptr += sizeof(pcaprec_hdr_t) + header->len;
				pcap->data_len -= sizeof(pcaprec_hdr_t) + header->len;

			    return pcap->data_ptr - header->len;
}
