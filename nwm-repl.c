/* nwm - a programmable window manager
 * Copyright (C) 2010  Nathan Sullivan
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either version 2 
 * of the License, or (at your option) any later version. 
 *
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  
 * 02110-1301, USA 
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <readline/readline.h>
#include <readline/history.h>

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_un remote;

    fprintf(stderr, "creating socket\n");
    sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    bzero(&remote, sizeof(remote));
    remote.sun_family = AF_LOCAL;

    char *home_path = getenv("HOME");
    char *sock_path = (char *)malloc((strlen(home_path) + 9) * sizeof(char));
    strcpy(sock_path, home_path);
    strcat(sock_path, "/.nwm/sock");
    strcpy(remote.sun_path, sock_path);
    free(sock_path);

    fprintf(stderr, "connecting to %s\n", remote.sun_path);
    if (connect(sockfd, (struct sockaddr *)&remote, sizeof(remote)) < 0) {
        perror("connect failed");
        exit(1);
    }

    char *line = NULL;
    char buf[4096];
    while ((line = readline("nwm-repl# "))) {
        if (*line) { /* not an empty line */
            write(sockfd, line, strlen(line));
            add_history(line);
            if (read(sockfd, buf, 4096) > 0)
                fprintf(stderr, "%s", buf);
        }
        free(line);
    }

    fprintf(stderr, "\n");
    close(sockfd);

    exit(0);
}
