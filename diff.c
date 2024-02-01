

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

#define MAX 128
#define MALFAIL(a) if (a == NULL) { \
			printf("Malloc Failed.\n"); \
			exit(1); \
		}

void usage(void) {
	printf("DIFF Implementation\nThis programs compares two files.\n");
	printf("The command line address should be: './project filename1 filename2' \n");
}

const int MOD_CS = 65521;

typedef struct file {
	uint32_t cksum;
	int line;
}file;

uint32_t checksum (char *line, size_t len) {
	uint32_t a = 1, b = 0;
	size_t index;

	for (index = 0; index < len; index++) {
		a = (a + line[index]) % MOD_CS;
		b = (b + a) % MOD_CS;
	}

	return (b << 16) | a;
}

typedef struct track {
	int *visit1, *visit2;
}track;

void t_init (track t, int s1, int s2) {
	int l;

	for (l = 0; l < s1; l++)
		t.visit1[l] = 0;

	for (l = 0; l < s2; l++)
		t.visit2[l] = 0;

	return;
}

track diff (file *cs1, file *cs2, int size_cs1, int size_cs2) {
	int k, i;

	track dif;
	dif.visit1 = (int *)malloc(sizeof(int) * size_cs1);
	MALFAIL(dif.visit1);
	dif.visit2 = (int *)malloc(sizeof(int) * size_cs2);
	MALFAIL(dif.visit2);

	t_init(dif, size_cs1, size_cs2);

	i = 0; k = 0;
	while (k < size_cs1) {
		while (i < size_cs2) {
			if (cs2[i].cksum == cs1[k].cksum) { //found
				dif.visit2[i] = 1;
				dif.visit1[k] = 1;
				break;
			} else
				i++;
		} //not found
		k++;
		i = 0;
	}

	while (i < size_cs2) { //ignoring blank lines of file2
		if (cs2[i].cksum == 1)
			dif.visit2[i] = 1;
		i++;
	}

	return dif;
}

int SIZE = 8;

typedef struct ndata {
	char ch, *str;
	int lnum;
	struct ndata *next;
}ndata;

ndata **ndiff;

void ninit () {
	ndiff = (ndata **)calloc(SIZE, sizeof(struct ndata *) * SIZE);
	MALFAIL(ndiff);
	int i;
	for (i = 0; i < SIZE; i++)
		ndiff[i] = NULL;
}

void nstore (ndata d, int i) {
	ndata *tmp, *p;

	tmp = (ndata *)malloc(sizeof(ndata));
	MALFAIL(tmp);
	*tmp = d;

	p = ndiff[i];
	if (p == NULL)
		ndiff[i] = tmp;
	else {
		while (p->next != NULL)
			p = p->next;
		p->next = tmp;
	}

	return;
}

void ndiffsort (track dif, int l1, int l2, char *f1[], char *f2[]) {
	int i, j, k, z;
	ndata d;
	ninit();

	k = 0; i = 0; j = 0;
	while (i < l1 && j < l2) {
		if (dif.visit1[i] == 0) {
			if (dif.visit2[j] == 1) {
				while (dif.visit1[i] == 0) {
					d.lnum = i + 1;
					d.ch = '-';
					d.str = f1[i];
					d.next = NULL;
					nstore(d, k);
					i++;
				}
				k++;
			} else {
				while (dif.visit1[i] == 0) {
					d.lnum = i + 1;
					d.ch = '-';
					d.str = f1[i];
					d.next = NULL;
					nstore(d, k);
					i++;
				}
				while (dif.visit2[j] == 0) {
					d.lnum = j + 1;
					d.ch = '+';
					d.str = f2[j];
					d.next = NULL;
					nstore(d, k);
					j++;
				}
				k++;
			}
		} else {
			if (dif.visit2[j] == 0) {
				while (dif.visit2[j] == 0) {
					d.lnum = j + 1;
					d.ch = '+';
					d.str = f2[j];
					d.next = NULL;
					nstore(d, k);
					j++;
				}
				k++;
			}
		}
		i++;
		j++;
		if (k == SIZE) {
			SIZE *= 2;
			ndiff = (ndata **)realloc(ndiff, sizeof(ndata *) * SIZE);
			for (z = k; z < SIZE; z++)
				ndiff[z] = NULL;
		}
	}

	while (j < l2) {
		if (dif.visit2[j] == 1)
			j++;
		else {
			while (dif.visit2[j] == 0) {
				d.lnum = j + 1;
				d.ch = '+';
				d.str = f2[j];
				d.next = NULL;
				nstore(d, k);
				j++;
				if (j == l2)
					break;
			}
			k++;
			if (k == SIZE) {
				SIZE *= 2;
				ndiff = (ndata **)realloc(ndiff, sizeof(ndata *) * SIZE);
				for (z = k; z < SIZE; z++)
					ndiff[z] = NULL;
			}
		}
	}

	while (i < l1) {
		if (dif.visit1[i] == 1)
			i++;
		else {
			while (dif.visit1[i] == 0) {
				d.lnum = i + 1;
				d.ch = '-';
				d.str = f1[i];
				d.next = NULL;
				nstore(d, k);
				i++;
				if (i == l1)
					break;
			}
			k++;
			if (k == SIZE) {
				SIZE *= 2;
				ndiff = (ndata **)realloc(ndiff, sizeof(ndata *) * SIZE);
				for (z = k; z < SIZE; z++)
					ndiff[z] = NULL;
			}
		}
	}

}

void printndiff() {
	int j;
	ndata *p, *q;

	j = 0;
	p = ndiff[j];

	while (p) {
		if (p->ch == '-') {
			if (p->next == NULL)
				printf("%dd\n< %s", p->lnum, p->str);
			else {
				q = p;
				while (q->ch == '-')
					q = q->next;
				printf("%dc%d\n", p->lnum, q->lnum);
				while (p->ch == '-') {
					printf("< %s", p->str);
					p = p->next;
				}
				printf("-----\n");
				while (p) {
					printf("> %s", p->str);
					p = p->next;
				}
			}
		} else {
			printf("%da\n", p->lnum);
			while (p) {
				printf("> %s", p->str);
				p = p->next;
			}
		}
		j++;
		p = ndiff[j];
	}

}

int main(int argc, char *argv[]) {

	if (strcmp(argv[1], "-h") == 0) {
		usage();
		return 0;
	} else {
// program 

	int i, j, size, size_f, len1, len2, n1, n2;
	char *line, *p, **f2, **f1;
	char line_char;
	size_t p_size;
	file *file1, *file2;
	FILE *fd1, *fd2;
	track dif;

	size = 20;
	size_f = 10;
	line_char = '\0';
	p = NULL;
	p_size = 0;

	if (argc != 3) {
		printf("Address of the type: ./program file1 file2\n");
		return EINVAL;
	}

	fd1 = fopen(argv[1], "r");
	if (fd1 == NULL) {
		perror("Can't open file1: ");
		return errno;
	}

	fd2 = fopen(argv[2], "r");
	if (fd2 == NULL) {
		perror("Can't open file2: ");
		return errno;
	}

	file1 = (file *)malloc(sizeof(file) * size_f);
	MALFAIL(file1);
	line = (char *)malloc(sizeof(char) * size);
	MALFAIL(line);
	j = 0;
	while (!feof(fd1)) {
		i = 0;
		fread(&line_char, 1, 1, fd1);
		while (line_char != '\n') {
			line[i] = line_char;
			i++;
			if (i == size) {
				size *= 2;
				line = (char *)realloc(line, sizeof(char) * size);
			}
			 fread(&line_char, 1, 1, fd1);
		}
		line[i] = '\0';
		file1[j].cksum = checksum(line, i);
		file1[j].line = j + 1;
		j++;
		if (j == size_f) {
			size_f *= 2;
			file1 = (file *)realloc(file1, sizeof(file) * size_f);
		}
	}
	len1 = j - 1;

	file2 = (file *)malloc(sizeof(file) * size_f);
	MALFAIL(file2);
	j = 0;
	while (!feof(fd2)) {
		i = 0;
		fread(&line_char, 1, 1, fd2);
		while (line_char != '\n') {
			line[i] = line_char;
			i++;
			if (i == size) {
				size *= 2;
				line = (char *)realloc(line, sizeof(char) * size);
			}
			fread(&line_char, 1, 1, fd2);
		}
		line[i] = '\0';
		file2[j].cksum = checksum(line, i);
		file2[j].line = j + 1;
		j++;
		if (j == size_f) {
			size_f *= 2;
			file2 = (file *)realloc(file2, sizeof(file) * size_f);
		}
	}
	len2 = j - 1;

	dif = diff (file1, file2, len1, len2);

	f1 = (char **)malloc(sizeof(char *) * size_f);
	MALFAIL(f1);
	i = 0; j = 0;
	rewind(fd1);
	while (!feof(fd1)) {			
		getline(&p, &p_size, fd1);
		f1[j] = (char *)malloc(sizeof(char) * p_size);
		MALFAIL(f1[j]);
		strcpy(f1[j], p);
		j++;
		if (j == size_f) {
			size_f *= 2;
			f1 = (char **)realloc(f1, sizeof(char *) * size_f);
		}
		i++;
	}
	n1 = j;

	f2 = (char **)malloc(sizeof(char *) * size_f);
	MALFAIL(f2);
	i = 0; j = 0;
	rewind(fd2);
	while (!feof(fd2)) {			
		getline(&p, &p_size, fd2);
		f2[j] = (char *)malloc(sizeof(char) * p_size);
		MALFAIL(f2[j]);
		strcpy(f2[j], p);
		j++;
		if (j == size_f) {
			size_f *= 2;
			f2 = (char **)realloc(f2, sizeof(char *) * size_f);
		}
		i++;
	}
	n2 = j;

	ndiffsort(dif, len1, len2, f1, f2);
	printndiff();

	for (i = 0; i < n1; i++)
		free(f1[i]);
	for (i = 0; i < n2; i++)
		free(f2[i]);
	free(f1);
	free(f2);
	free(file1);
	free(file2);
	free(line);
	free(p);
	free(dif.visit1);
	free(dif.visit2);
	fclose(fd1);
	fclose(fd2);

	return 0;

	} // end else
}
