/*  c++ program that reads a pgm type grayscale image 
and write it's decimal pixel in a text file */
#include<iostream>
#include "imageHeader.h"
using namespace std;

void ignoreComments(FILE* fp)
{
	int ch;
	char line[100];

	// Ignore any blank lines
	while ((ch = fgetc(fp)) != EOF && isspace(ch));

	// Recursively ignore comments in a PGM image commented lines start with a '#'
	if (ch == '#')
    {
		fgets(line, sizeof(line), fp);
		ignoreComments(fp);
	}
	else
		fseek(fp, -1, SEEK_CUR); // adjusts if an extra character is read
}

// Function to open the input a PGM file and process it
bool readPGMImage(PGMImage* pgm,const char* filename)
{
	// Open the image file in the 'read binary' mode
	FILE* pgmfile = fopen(filename, "rb");

	// If file does not exist,then return
	if (pgmfile == NULL) {
		printf("File does not exist\n");
		return false;
	}

	ignoreComments(pgmfile);
	fscanf(pgmfile, "%s",
		pgm->pgmType);

	// Check for correct PGM Binary file type
	if (strcmp(pgm->pgmType, "P5")) {
		fprintf(stderr,"Wrong file type!\n");
		exit(EXIT_FAILURE);
	}

	ignoreComments(pgmfile);

	// Read the image dimensions
	fscanf(pgmfile, "%d %d",&(pgm->width),&(pgm->height));

	ignoreComments(pgmfile);

	// Read maximum gray value
	fscanf(pgmfile, "%d", &(pgm->maxColorValue));
	ignoreComments(pgmfile);

	// Allocating memory to store image info in defined struct
	pgm->data = (unsigned char**)malloc(pgm->height * sizeof(unsigned char*));

	// Storing the pixel info in the struct
	if (pgm->pgmType[1] == '5') {

		fgetc(pgmfile);

		for (int i = 0; i < pgm->height; i++) {
			pgm->data[i] = (unsigned char*)malloc(pgm->width * sizeof(unsigned char));

			// If memory allocation is failed
			if (pgm->data[i] == NULL) {
				fprintf(stderr,"malloc failed\n");
				exit(1);
			}

			// Read the gray values and write on allocated memory
			fread(pgm->data[i],sizeof(unsigned char),pgm->width, pgmfile);
		}
	}

	// Close the file
	fclose(pgmfile);

	return true;
}
// Function to print the file details
void printImageDetailsPGM(PGMImage* pgm, const char* filename,const char* filename2)
{
	FILE* pgmfile = fopen(filename, "rb");
	FILE* outputFile = fopen(filename2, "w");
	if (outputFile == NULL)
    {
        printf("Could not open file");
    }
	// Retrieving the file extension
	const char* ext = strrchr(filename, '.');

	if (!ext)
		printf("No extension found in file %s",filename);
	else
		printf("File format"" : %s\n",ext + 1);

	fprintf(outputFile,"PGM File type : %s\n",pgm->pgmType);

	// Print type of PGM file, in ascii and binary format
	if (!strcmp(pgm->pgmType, "P2"))
		printf("PGM File Format: ASCII\n");
	else if (!strcmp(pgm->pgmType,"P5"))
		printf("PGM File Format: Binary\n");
	
	printf("Width of image : %d \n",pgm->width);
	printf("Height of image : %d \n",pgm->height);
	printf("Max Gray value : %d\n",pgm->maxColorValue);

    // Printing image information in the output file
	fprintf(outputFile,"Width of image : %d \n",pgm->width);
	fprintf(outputFile,"Height of image : %d \n",pgm->height);
	fprintf(outputFile,"Max Gray value : %d\n",pgm->maxColorValue);
	for(int i = 0 ; i < pgm->height; i++)
    {
        for(int j = 0; j < pgm->width; j++)
        {
            fprintf(outputFile,"%d ", pgm->data[i][j]);
        }

    }

	// close file
	fclose(pgmfile);
	fclose(outputFile);
}

// Constracting image from the retrived data
void constructPGMImage(PGMImage* pgm, char const* filename)
{
    FILE* pgmOut = fopen(filename, "wb");
    fprintf(pgmOut,"%c%c\n",pgm->pgmType[0],pgm->pgmType[1]);
    //fprintf(pgmOut,"\n");
    fprintf(pgmOut,"%d %d\n",pgm->width,pgm->height);
    fprintf(pgmOut,"%d\n",pgm->maxColorValue);

    for(int i =0; i < pgm->height; i++)
    {
        fwrite(pgm->data[i],sizeof(unsigned char),pgm->width, pgmOut);
    }

    fclose(pgmOut);
}

// Driver Code
int main(int argc, char const* argv[])
{
	//PGMImage* pgm = malloc(sizeof(PGMImage));
	PGMImage* pgm = new PGMImage;
	const char* ipfile;
	const char* opfile;

	if (argc == 2)
		ipfile = argv[1];
	else
		ipfile = "baboon.pgm";
	
	opfile = "outOishi.txt";
	printf("Input file : %s\n", ipfile);

	// Process the image and print its details
	if (readPGMImage(pgm, ipfile))
	{
		printImageDetailsPGM(pgm, ipfile, opfile);
		constructPGMImage(pgm,"testOut.pgm");
	}
	return 0;
}