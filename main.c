#include <stdio.h>
#include "compression.h"
// #include "decompress.h"

int main(void){
    int choice1, choice2;
    int yn;
    printf("Welcome to image compressor tool.\n\nWhat do you want to do??\n1.Compress an image.\n2.Decompress an image.\n");
    printf("Enter your choice in number: ");
    scanf("%d", &yn);
    if (yn == 1)
    {
        printf("\nWhat is your image type??\n1.PGM image.\n2.BMP image.\n3.JPEG image.\n");
        printf("Enter your choice in number: ");
        scanf("%d", &choice1);
        printf("\n");
        if(choice1 == 1){
            printf("Which algorithm you want to use??\n1.Huffman coding.\n2.Run Length Encoding.\n3.LZW.\n");
            printf("Enter your choice in number: ");
            scanf("%d", &choice2);
            printf("\n");
            if (choice2 == 1)
            {
                compressHuffmanPgm();
            
            }
            else if(choice2 == 2){
                //compressRunLengthPGM();
                rle();
            }
            else if(choice2 == 3)
            {
                compressLZWP();
            }
            else{
                printf("Invalid choice.\n");
            }    

        }
        else if(choice1 == 2){
            printf("Which algorithm you want to use??\n1.Huffman coding.\n2.Run Length Encoding.\n3.LZW.\n");
            printf("Enter your choice in number: ");
            scanf("%d", &choice2);
            if (choice2 == 1)
            {
                Huffman();
            } 
            else if(choice2 == 2){
                RLE();
            }
            else if(choice2 == 3)
            {
                LZW();
            }
            else{
                printf("Invalid choice.\n");
            }

        }
        else if(choice1 == 3){
            printf("For JPEG image, we are using DCT algorithm.\n");
            DCT();  
        }
        else{
            printf("Invalid choice.\n");
        }
    }
    else if (yn == 2)
    {
        printf("Which algorithm you have used??\n1.Huffman coding.\n2.Run Length Encoding.\n3.LZW.\n");
        printf("Enter your choice in number: ");
        scanf("%d", &choice2);
        if (choice2 == 1)
        {
            //decompressHuffmanPgm();
        } 
        else if(choice2 == 2){
            rle();
        }
        else if(choice2 == 3)
        {
            decomLzw(); 
        }
        else{
            printf("Invalid choice.\n");
        }
        
    }
    else{
        printf("Invalid choice.\n");
    }
    printf("\n");
    printf("Thank you for using the Image Compression Tool");
    return 0;
} 
