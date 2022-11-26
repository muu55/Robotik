char color(int light, int red, int green, int blue){
    int rgbvalue = red + green + blue;
    rgbvalue = rgbvalue / 3;

    if ((light >= 60) && (light <= 80) && (rgbvalue >= 15) && (rgbvalue <= 22)){
        char color = 'w'; //white
        return color;
    }
    else if ((light >= 30) && (light <= 45) && (rgbvalue >= 12) && (rgbvalue <= 22)){
        char color = 'r'; // red
	return color;
    }
    else if ((light <= 15) && (rgbvalue <= 4)){
        char color = 'b'; // black 
        return color;
    }
    else if ((light >= 100) && (light <= 170) && (rgbvalue >= 40)){
        char color = 's'; // silver
        return color;
    }
    else{
        return 'e';
    }
}
