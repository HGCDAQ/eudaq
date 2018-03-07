#include "HexaPos.h"
#include <iostream>

/* Constructor and destructor */

HexaPos::HexaPos() {}

HexaPos::HexaPos(double centerX, double centerY) {

    m_centerX = centerX;
    m_centerY = centerY;
}

HexaPos::~HexaPos() {}

/* Construct position grid */
int HexaPos::BuildGrid() {

    /*
     _______
    /   |   \
   /    |    \
  /_____|_____\
  \     |     /
   \    |    /
    \   |   /
     -------

    */

    double posX = m_startX;
    double posY = m_startY;

    while(posY > - (m_smallR + m_borderOffsetY)){

	if ((posY >= 0. && ( posY < m_a * posX + m_b )) ||
	    (posY < 0. && ( posY > -(m_a * posX + m_b)))) {

	    m_npoints += 1;

	    m_positionsX.push_back(posX + m_centerX);
	    m_positionsY.push_back(posY + m_centerY);
	    //std::vector<double> posXY{ posX, posY};
	    //m_positions.push_back(posXY);
	}


	// Move
	if (posY > 0){
	    // Reached end of row, moving to the start of the next one
	    if ( posY > m_a * posX + m_b){
		posY -= m_stepSizeY;
		if ( posY > 0 )
		    posX = - (posY - m_b) / m_a + m_borderOffsetX;
		else
		    posX = (posY + m_b) / m_a + m_borderOffsetX;
	    }
	    else {
		posX += m_stepSizeX;
	    }
	}
	else {
	    // Reached end of row, moving to the start of the next one
	    if ( posY < -(m_a * posX + m_b)){
		posY -= m_stepSizeY;
		posX = (posY + m_b) / m_a + m_borderOffsetX;
	    }
	    else {
		posX += m_stepSizeX;
	    }
	}

	//if (m_npoints > 100) break;
    }

    return 0;
}
