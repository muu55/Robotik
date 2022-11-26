// function to get color status out of sensor values

char color(int light, int red, int green, int blue){
    int rgbvalue = red + green + blue;
    rgbvalue = rgbvalue / 3;
    //printf("RGBval: %d \n", rgbvalue);
    //printf("Red: %d \n", red);
    //printf("Green: %d \n", green);
    //printf("blue: %d \n", blue);

    if ((light >= 90) && (light <= 110) && (rgbvalue >= 15) && (rgbvalue <= 22)){
        char color = 'w'; //white
        return color;
    }
    else if ((light >= 30) && (light <= 45) && (rgbvalue >= 12) && (rgbvalue <= 22)){
        char color = 'r'; // red
	return color;
    }
    else if ((light <= 12) && (rgbvalue <= 4)){
        char color = 'b'; // black 
        return color;
    }
    else if ((light >= 111) && (light <= 140) && (rgbvalue >= 40)){
        char color = 's'; // silver
        return color;
    }
    else{
        return 'e'; // error
    }
}
