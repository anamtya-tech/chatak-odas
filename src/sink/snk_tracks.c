   
   /**
    * \file     snk_tracks.c
    *
    */
    
    #include <sink/snk_tracks.h>
    #include <sys/time.h>
    #include <stdarg.h>

    static void json_escape_string(const char *src, char *dst, size_t dstSize) {

        size_t si;
        size_t di;

        if (dstSize == 0) {
            return;
        }

        if (src == NULL) {
            dst[0] = '\0';
            return;
        }

        di = 0;
        for (si = 0; src[si] != '\0' && di + 1 < dstSize; si++) {

            char c = src[si];

            if (c == '\\' || c == '"') {
                if (di + 2 >= dstSize) {
                    break;
                }
                dst[di++] = '\\';
                dst[di++] = c;
            }
            else if (c == '\n' || c == '\r' || c == '\t') {
                dst[di++] = ' ';
            }
            else {
                dst[di++] = c;
            }

        }

        dst[di] = '\0';

    }

    static void append_to_buffer(char *buffer, size_t bufferCapacity, const char *fmt, ...) {

        size_t used;
        va_list args;
        int n;

        used = strlen(buffer);
        if (used >= bufferCapacity) {
            return;
        }

        va_start(args, fmt);
        n = vsnprintf(buffer + used, bufferCapacity - used, fmt, args);
        va_end(args);

        if (n < 0) {
            buffer[used] = '\0';
        }

    }

    snk_tracks_obj * snk_tracks_construct(const snk_tracks_cfg * snk_tracks_config, const msg_tracks_cfg * msg_tracks_config) {

        snk_tracks_obj * obj;

        obj = (snk_tracks_obj *) malloc(sizeof(snk_tracks_obj));

        obj->timeStamp = 0;

        obj->nTracks = msg_tracks_config->nTracks;
        obj->fS = snk_tracks_config->fS;
        
        obj->format = format_clone(snk_tracks_config->format);
        obj->interface = interface_clone(snk_tracks_config->interface);

        if (!(((obj->interface->type == interface_blackhole)  && (obj->format->type == format_undefined)) ||
              ((obj->interface->type == interface_file)  && (obj->format->type == format_text_json)) ||
              ((obj->interface->type == interface_socket)  && (obj->format->type == format_text_json)) ||
              ((obj->interface->type == interface_terminal) && (obj->format->type == format_text_json)))) {
            
            interface_printf(obj->interface);
            format_printf(obj->format);

            printf("Sink tracks: Invalid interface and/or format.\n");
            exit(EXIT_FAILURE);

        }

        obj->fp = (FILE *) NULL;

        obj->buffer = (char *) malloc(sizeof(char) * 4096);
        memset(obj->buffer, 0x00, sizeof(char) * 4096);
        obj->bufferSize = 0;

        obj->in = (msg_tracks_obj *) NULL;

        return obj;

    }

    void snk_tracks_destroy(snk_tracks_obj * obj) {

        free((void *) obj->buffer);

        format_destroy(obj->format);
        interface_destroy(obj->interface);

        free((void *) obj);

    }

    void snk_tracks_connect(snk_tracks_obj * obj, msg_tracks_obj * in) {

        obj->in = in;

    }

    void snk_tracks_disconnect(snk_tracks_obj * obj) {

        obj->in = (msg_tracks_obj *) NULL;

    }

    void snk_tracks_open(snk_tracks_obj * obj) {

        switch(obj->interface->type) {

            case interface_blackhole:

                snk_tracks_open_interface_blackhole(obj);

            break;

            case interface_file:

                snk_tracks_open_interface_file(obj);

            break;

            case interface_socket:

                snk_tracks_open_interface_socket(obj);

            break;

            case interface_terminal:

                snk_tracks_open_interface_terminal(obj);

            break;

            default:

                printf("Sink tracks: Invalid interface type.\n");
                exit(EXIT_FAILURE);

            break;           

        }

    }

    void snk_tracks_open_interface_blackhole(snk_tracks_obj * obj) {

        // Empty

    }

    void snk_tracks_open_interface_file(snk_tracks_obj * obj) {

        obj->fp = fopen(obj->interface->fileName, "wb");

        if (obj->fp == NULL) {
            printf("Cannot open file %s\n",obj->interface->fileName);
            exit(EXIT_FAILURE);
        }

    }

    void snk_tracks_open_interface_socket(snk_tracks_obj * obj) {

        memset(&(obj->sserver), 0x00, sizeof(struct sockaddr_in));

        obj->sserver.sin_family = AF_INET;
        obj->sserver.sin_addr.s_addr = inet_addr(obj->interface->ip);
        obj->sserver.sin_port = htons(obj->interface->port);
        obj->sid = socket(AF_INET, SOCK_STREAM, 0);

        if ( (connect(obj->sid, (struct sockaddr *) &(obj->sserver), sizeof(obj->sserver))) < 0 ) {

            printf("Sink tracks: Cannot connect to server\n");
            exit(EXIT_FAILURE);

        }   

    }

    void snk_tracks_open_interface_terminal(snk_tracks_obj * obj) {

        // Empty

    }

    void snk_tracks_close(snk_tracks_obj * obj) {

        switch(obj->interface->type) {

            case interface_blackhole:

                snk_tracks_close_interface_blackhole(obj);

            break;

            case interface_file:

                snk_tracks_close_interface_file(obj);

            break;

            case interface_socket:

                snk_tracks_close_interface_socket(obj);

            break;

            case interface_terminal:

                snk_tracks_close_interface_terminal(obj);

            break;

            default:

                printf("Sink tracks: Invalid interface type.\n");
                exit(EXIT_FAILURE);

            break;

        }

    }

    void snk_tracks_close_interface_blackhole(snk_tracks_obj * obj) {

        // Empty

    }

    void snk_tracks_close_interface_file(snk_tracks_obj * obj) {

        fclose(obj->fp);

    }

    void snk_tracks_close_interface_socket(snk_tracks_obj * obj) {

        close(obj->sid);

    }

    void snk_tracks_close_interface_terminal(snk_tracks_obj * obj) {

        // Empty

    }

    int snk_tracks_process(snk_tracks_obj * obj) {

        int rtnValue;

        if (obj->in->timeStamp != 0) {

            switch(obj->format->type) {

                case format_text_json:

                    snk_tracks_process_format_text_json(obj);

                break;

                case format_undefined:

                    snk_tracks_process_format_undefined(obj);

                break;

                default:

                    printf("Sink tracks: Invalid format type.\n");
                    exit(EXIT_FAILURE);

                break;                

            }

            switch(obj->interface->type) {

                case interface_blackhole:

                    snk_tracks_process_interface_blackhole(obj);

                break;

                case interface_file:

                    snk_tracks_process_interface_file(obj);

                break;

                case interface_socket:

                    snk_tracks_process_interface_socket(obj);

                break;

                case interface_terminal:

                    snk_tracks_process_interface_terminal(obj);

                break;

                default:

                    printf("Sink tracks: Invalid interface type.\n");
                    exit(EXIT_FAILURE);

                break;

            }

            rtnValue = 0;

        }
        else {

            rtnValue = -1;

        }

        return rtnValue;

    }

    void snk_tracks_process_interface_blackhole(snk_tracks_obj * obj) {

        // Empty

    }

    void snk_tracks_process_interface_file(snk_tracks_obj * obj) {

        fwrite(obj->buffer, sizeof(char), obj->bufferSize, obj->fp);

    }

    void snk_tracks_process_interface_socket(snk_tracks_obj * obj) {

        if (send(obj->sid, obj->buffer, obj->bufferSize, 0) < 0) {
            printf("Sink tracks: Could not send message.\n");
            exit(EXIT_FAILURE);
        }  

    }

    void snk_tracks_process_interface_terminal(snk_tracks_obj * obj) {

        printf("%s",obj->buffer);

    }

    void snk_tracks_process_format_text_json(snk_tracks_obj * obj) {

        unsigned int iTrack;
        struct timeval tv;
        char escapedTag[512];
        char escapedClass[512];
        gettimeofday(&tv, NULL);
        unsigned long long systemTimeMs = (unsigned long long)(tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;

        obj->buffer[0] = 0x00;

        append_to_buffer(obj->buffer, 4096, "{\n");
        append_to_buffer(obj->buffer, 4096, "    \"timeStamp\": %llu,\n", systemTimeMs);
        append_to_buffer(obj->buffer, 4096, "    \"src\": [\n");

        for (iTrack = 0; iTrack < obj->nTracks; iTrack++) {

            const char *cname = (obj->in->tracks->class_name &&
                                 obj->in->tracks->class_name[iTrack][0])
                                ? obj->in->tracks->class_name[iTrack] : "";
            float cconf = obj->in->tracks->class_conf
                          ? obj->in->tracks->class_conf[iTrack] : 0.0f;

            json_escape_string(obj->in->tracks->tags[iTrack], escapedTag, sizeof(escapedTag));
            json_escape_string(cname, escapedClass, sizeof(escapedClass));

            append_to_buffer(obj->buffer,
                4096,
                "{ \"id\": %llu, \"tag\": \"%s\","
                " \"x\": %1.3f, \"y\": %1.3f, \"z\": %1.3f,"
                " \"activity\": %1.3f,"
                " \"class\": \"%s\", \"class_conf\": %1.3f }",
                obj->in->tracks->ids[iTrack],
                escapedTag,
                obj->in->tracks->array[iTrack*3+0],
                obj->in->tracks->array[iTrack*3+1],
                obj->in->tracks->array[iTrack*3+2],
                obj->in->tracks->activity[iTrack],
                escapedClass,
                cconf
            );

            /* Live console display for active tracks */
            if (obj->in->tracks->activity[iTrack] > 0.0f) {
                if (cname[0]) {
                    fprintf(stderr,
                        "[SST] id=%llu  x=%+.2f y=%+.2f z=%+.2f  act=%.2f  | %s (%.0f%%)\n",
                        obj->in->tracks->ids[iTrack],
                        obj->in->tracks->array[iTrack*3+0],
                        obj->in->tracks->array[iTrack*3+1],
                        obj->in->tracks->array[iTrack*3+2],
                        obj->in->tracks->activity[iTrack],
                        cname,
                        cconf * 100.0f);
                } else {
                    fprintf(stderr,
                        "[SST] id=%llu  x=%+.2f y=%+.2f z=%+.2f  act=%.2f  | (classifying...)\n",
                        obj->in->tracks->ids[iTrack],
                        obj->in->tracks->array[iTrack*3+0],
                        obj->in->tracks->array[iTrack*3+1],
                        obj->in->tracks->array[iTrack*3+2],
                        obj->in->tracks->activity[iTrack]);
                }
            }

            if (iTrack != (obj->nTracks - 1)) {

                append_to_buffer(obj->buffer, 4096, ",");

            }

            append_to_buffer(obj->buffer, 4096, "\n");

        }
        
        append_to_buffer(obj->buffer, 4096, "    ]\n");
        append_to_buffer(obj->buffer, 4096, "}\n");

        obj->bufferSize = strlen(obj->buffer);

    }

    void snk_tracks_process_format_undefined(snk_tracks_obj * obj) {

        obj->buffer[0] = 0x00;
        obj->bufferSize = 0;

    }

    snk_tracks_cfg * snk_tracks_cfg_construct(void) {

        snk_tracks_cfg * cfg;

        cfg = (snk_tracks_cfg *) malloc(sizeof(snk_tracks_cfg));

        cfg->fS = 0;
        cfg->format = (format_obj *) NULL;
        cfg->interface = (interface_obj *) NULL;

        return cfg;

    }

    void snk_tracks_cfg_destroy(snk_tracks_cfg * snk_tracks_config) {

        if (snk_tracks_config->format != NULL) {
            format_destroy(snk_tracks_config->format);
        }
        if (snk_tracks_config->interface != NULL) {
            interface_destroy(snk_tracks_config->interface);
        }

        free((void *) snk_tracks_config);

    }
