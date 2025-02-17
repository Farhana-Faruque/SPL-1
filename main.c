#include<stdio.h>
#include "compression.h"

int main(void){
    char filename[256];
    int choice1, choice2;
    printf("Welcome to image compressor tool.\nWhat is your image type??\n1.BMP image.\n2.PGM image.\n3.JPEG image.\n");
    printf("Enter your choice in number: ");
    scanf("%d", &choice1);
    if(choice1 == 1){
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
           // LZW();
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
            
        }
        else if(choice2 == 2){
            
        }
        else if(choice2 == 3)
        {
            
        }
        else{
            printf("Invalid choice.\n");
        }
    }
    else if(choice1 == 3){
        printf("For JPEG image, we are using DCT algorithm.\n");
        printf("Enter your JPEG file name: ");
        scanf("%255s", filename);
    }
    else{
        printf("Invalid choice.\n");
    }
    return 0;
}