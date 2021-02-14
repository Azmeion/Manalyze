#include "icon_analysis.h"
#include "lodepng.h"
#include <fstream>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <math.h>
#include <iostream>

const BYTE PNG_SIG[8] = {137,80,78,71,13,10,26,10};

dataIcon::dataIcon(const std::vector<RGBQUAD> & rgba_colors, const WORD & dim) : dim(dim), mean(0), variance(0) {

    grayTable.reserve(dim * dim);

    float luma,r,g,b;
    for (int i = 0; i < dim; ++i) {
        for (int j = 0; j < dim; ++j) {

            r = (float) rgba_colors[i * dim + j].rgbRed;
            g = (float) rgba_colors[i * dim + j].rgbGreen;
            b = (float) rgba_colors[i * dim + j].rgbBlue;

            luma = 0.2126f * r + 0.7152f * g + 0.0722f * b;
            mean += luma;
            grayTable.push_back((BYTE) luma);
        }
    }

    mean /= dim*dim;

    for (BYTE gray : grayTable) {
        luma = gray - mean;
        variance += luma * luma;
    }

    variance /= (dim * dim - 1);
}

float dataIcon::ssim(dataIcon & database_icon) {
    
    float cov = 0.0, sim;
    float c1 = 0.01f * (dim - 1);
    c1 = c1*c1;
    float c2 = 0.03f * (dim - 1);
    c2 = c2*c2;

    std::vector<unsigned int>::iterator di_it = database_icon.grayTable.begin();

    //Sample covariance calculation
    for (unsigned int i_it : grayTable) {
        cov += (i_it - mean) * (*di_it - database_icon.mean); //
        di_it++;
    }

    cov /= dim * dim - 1; // Equivalent probabilities in every point.
    std::cout << cov << std::endl;

    sim = (2 * mean * database_icon.mean + c1) * (2 * abs(cov) + c2);
    sim /= (mean * mean + database_icon.mean * database_icon.mean + c1) * (variance + database_icon.variance + c2);

    return sim;
}

void icoHeaderRead(ICONDIR * pIconDir, const std::vector<BYTE> & buffer, int & nbBytesRead) {
    // On pourrait aussi faire des constructeurs à la place des memcpy.
    memcpy(&(pIconDir->idReserved), &buffer[nbBytesRead], sizeof(WORD)); //Must be 0. (Reserved bytes.)
    nbBytesRead += sizeof(WORD);

    memcpy(&(pIconDir->idType), &buffer[nbBytesRead], sizeof(WORD)); // Must be 1. (ICO type.)
    nbBytesRead += sizeof(WORD);

    memcpy(&(pIconDir->idCount), &buffer[nbBytesRead], sizeof(WORD)); // Number of images.
    nbBytesRead += sizeof(WORD);

    // Read the ICONDIRENTRY elements (size 16)
    for (LONG i = 0; i<pIconDir->idCount ; ++i)
    {
        // char * elt = new char [sizeof(ICONDIRENTRY)];
        // memcpy(elt, buffer+nbBytesRead, sizeof(ICONDIRENTRY));
        // nbBytesRead += sizeof(ICONDIRENTRY);
        // ICONDIRENTRY icon(elt);
        pIconDir->idEntries.push_back(*((ICONDIRENTRY*)(&buffer[nbBytesRead])));
        nbBytesRead += sizeof(ICONDIRENTRY);
        // delete elt;
    }
}

template<typename T> 
int vectorCpy(std::vector<T> & vector, const unsigned long & nbrPixel, const std::vector<BYTE> & buffer, int & nbBytesRead){
    vector.reserve(nbrPixel);
    vector.assign((T*)(&buffer[nbBytesRead]), (T*)(&buffer[nbBytesRead+nbrPixel*sizeof(T)]));
    nbBytesRead+=nbrPixel*sizeof(T);
    return nbBytesRead;
};

Icon::Icon(ICONDIRENTRY & icondirentry, const std::vector<BYTE> & buffer, int & nbBytesRead) : png(false), nbrPixel(-1)
{
    unsigned bHeight = icondirentry.getbHeight();
    unsigned bWidth = icondirentry.getbWidth();
    WORD dim = bHeight;

    std::cout << bHeight << " ; " << bWidth << std::endl;
    if (!(bHeight % MIN_ICON_SIZE) && !(bWidth % MIN_ICON_SIZE)) // || !(bHeight || bWidth)
    {
        pIconDir = &icondirentry;
        // pIconImage->icHeader.biHeight.to_ullong()/2*pIconImage->icHeader.biWidth.to_ullong(); // Taille des masques.
        if (!(nbrPixel = bHeight*bWidth)) {
            nbrPixel = 65536;
            dim = 256;
        }

        nbBytesRead = icondirentry.getOffset();
        if (!strncmp((const char *)&buffer[nbBytesRead], (char*)PNG_SIG, 8))
        {
            png = true;
            std::vector<BYTE> image;
            unsigned error = lodepng::decode(image, bHeight, bWidth, (const unsigned char*)(&buffer[nbBytesRead]), (size_t)icondirentry.getBytesInRes());
            //if there's an error, display it
            if(error) std::cout << "PNG decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
            //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...

            std::vector<RGBQUAD> icColor;
            RGBQUAD * rgb = new RGBQUAD;
            for (std::vector<BYTE>::iterator p = image.begin() ; p!=image.end(); p++)
            {
                rgb->rgbRed = (*p);
                p++;
                rgb->rgbGreen = (*p);
                p++;
                rgb->rgbBlue = (*p);
                p++;
                rgb->rgbReserved = (*p);
                icColor.push_back(*rgb); //Is ok.
            }
            data = dataIcon(icColor, dim);
            delete rgb;
        }
        else 
        {
            ICONIMAGE * pIconImage = new ICONIMAGE;
            memcpy(&(pIconImage->icHeader), &buffer[nbBytesRead], sizeof(BITMAPINFOHEADER));
            nbBytesRead += sizeof(BITMAPINFOHEADER);

            // Experimental
            vectorCpy(pIconImage->icColors, nbrPixel, buffer, nbBytesRead);
            //vectorCpy(pIconImage->icXOR, nbrPixel, buffer, nbBytesRead);
            //vectorCpy(pIconImage->icAND, nbrPixel, buffer, nbBytesRead);
            data = dataIcon(pIconImage->icColors, dim);

            delete pIconImage;
        }
    }
}

std::vector<dataIcon> icoEntriesRead(const ICONDIR * pIconDir, const std::vector<BYTE> & buffer, int & nbBytesRead) {//, std::vector<dataIcon> & icoList) {
    std::vector<dataIcon> data;
    for (ICONDIRENTRY ico : pIconDir->idEntries)
    {
        Icon * i = new Icon(ico, buffer, nbBytesRead);
        if (i->getNbrPixel() != -1) { // Évite que data soit accidentellement purgé en attendant qu'on gère tous nos trucs.
            data.push_back(i->getData());
        }
        delete i;
    }
    std::sort(data.begin(), data.end(), icoSort);
    return data;
}

void fileToBuf(const char * fname, std::vector<BYTE> & buffer) // logiquement ça marche avec le vector
{
    std::ifstream is(fname, std::ios::binary);

    if(is)
    {
        is.seekg (0, is.end);
        int length = is.tellg();
        is.seekg (0, is.beg);

        buffer.reserve(length);
        buffer.assign(std::istreambuf_iterator<char>(is),
                        std::istreambuf_iterator<char>());
        
        // buffer = new char [length];
        // is.read(buffer, length);
    }
}

bool icoSort(const dataIcon & ico1, const dataIcon & ico2) {
    return ico1.getDim() > ico2.getDim();
}

/*

int main()
{
    // We need an ICONDIR to hold the data
    ICONDIR * pIconDir = new ICONDIR; // TODO : Utiliser des jolis pointeurs.
    ICONDIR * pIconDir2 = new ICONDIR; // TODO : Utiliser des jolis pointeurs.
    char * buffer = fileToBuf("C:\\Users\\Azuro\\Desktop\\Projet\\office.ico");
    char * buffer2 = fileToBuf("C:\\Users\\Azuro\\Desktop\\Projet\\office2.ico");
    dataIcon math, office;

    if (buffer)
    {
        int nbBytesRead = 0;
        icoHeaderRead(pIconDir, buffer, nbBytesRead);
        math = icoEntriesRead(pIconDir, buffer, nbBytesRead);
        delete pIconDir;
        delete buffer;
    }
    if (buffer2)
    {
        int nbBytesRead = 0;
        icoHeaderRead(pIconDir2, buffer2, nbBytesRead);
        office = icoEntriesRead(pIconDir2, buffer2, nbBytesRead);
        delete pIconDir2;
        delete buffer2;
    }
    if(math.getMean() && office.getMean())
    {
        float result = office.ssim(math);
        std::cout << result;
    }
    return 0;
}

*/