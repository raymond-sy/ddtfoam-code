/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright held by original author
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Application
    AUSMplusALEFlux

Author
    Oliver Borm  All rights reserved.

\*---------------------------------------------------------------------------*/

#include "AUSMplusALEFlux.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //+


// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::AUSMplusALEFlux::evaluateFlux
(
    scalar& rhoFlux,
    vector& rhoUFlux,
    scalar& rhoEFlux,
    scalarList& rhoScalarFlux,
 
    const scalar& pLeft,
    const scalar& pRight,
 
    const vector& rhoULeft,
    const vector& rhoURight,

    const scalar& rhoLeft,
    const scalar& rhoRight,

    const scalar& aLeft,
    const scalar& aRight,

    const scalar& rhoELeft,
    const scalar& rhoERight,
 
    const scalarList& rhoScalarLeft,
    const scalarList& rhoScalarRight,

    const vector& Sf,
    const scalar& magSf,
    const scalar& K_Roe
) const
{
        const scalar alpha = 3.0/16.0;
        const scalar beta = 1.0/8.0;

        // normal vector
        const vector normalVector = Sf/magSf;

	const scalar rhoHLeft  = rhoELeft  + pLeft;	// AUSM+ requires h flux, not e; see Liou paper
	const scalar rhoHRight = rhoERight + pRight;
	
        // Step 1: decode left and right:

        // Compute conservative variables assuming perfect gas law

        //const vector rhoULeft  = rhoLeft *ULeft;
        //const vector rhoURight = rhoRight*URight;
	const vector ULeft=rhoULeft/rhoLeft;
	const vector URight=rhoURight/rhoRight;

        // compute qLeft and qRight (q_{l,r} = U_{l,r} \bullet n)
        const scalar qLeft  = ULeft & normalVector;
        const scalar qRight = URight & normalVector;

        const scalar aTilde = 0.5*(aLeft+aRight);

        const scalar MaRelLeft  = qLeft /aTilde;
        const scalar MaRelRight = qRight/aTilde;

        const scalar magMaRelLeft  = mag(MaRelLeft);
        const scalar magMaRelRight = mag(MaRelRight);

        const scalar Ma1PlusLeft   = 0.5*(MaRelLeft +magMaRelLeft );
        const scalar Ma1MinusRight = 0.5*(MaRelRight-magMaRelRight);

        const scalar Ma2PlusLeft   =  0.25*sqr(MaRelLeft +1.0);
        const scalar Ma2PlusRight  =  0.25*sqr(MaRelRight+1.0);
        const scalar Ma2MinusLeft  = -0.25*sqr(MaRelLeft -1.0);
        const scalar Ma2MinusRight = -0.25*sqr(MaRelRight-1.0);

        const scalar Ma4BetaPlusLeft   = ((magMaRelLeft  >= 1.0) ? Ma1PlusLeft   : (Ma2PlusLeft  *(1.0-16.0*beta*Ma2MinusLeft)));
        const scalar Ma4BetaMinusRight = ((magMaRelRight >= 1.0) ? Ma1MinusRight : (Ma2MinusRight*(1.0+16.0*beta*Ma2PlusRight)));

        const scalar P5alphaPlusLeft   = ((magMaRelLeft  >= 1.0) ?
            (Ma1PlusLeft/MaRelLeft)    : (Ma2PlusLeft  *(( 2.0-MaRelLeft) -16.0*alpha*MaRelLeft *Ma2MinusLeft )));
        const scalar P5alphaMinusRight = ((magMaRelRight >= 1.0) ?
            (Ma1MinusRight/MaRelRight) : (Ma2MinusRight*((-2.0-MaRelRight)+16.0*alpha*MaRelRight*Ma2PlusRight)));

        const scalar MaRelTilde = Ma4BetaPlusLeft + Ma4BetaMinusRight;
        const scalar pTilde = pLeft*P5alphaPlusLeft + pRight*P5alphaMinusRight;

        const scalar URelTilde = MaRelTilde*aTilde;
        const scalar magURelTilde = mag(MaRelTilde)*aTilde;
        // There is a typo in Luo et. al, J. Comp. Physics 194 (2004), Chap 4.2 Eq. 4.8
        // refer to the origial Paper from Liou, J. Comp. Physics 129 (1996), Chap4, Eq. 42
        rhoFlux  = (0.5*(URelTilde*(rhoLeft +rhoRight) -magURelTilde*(rhoRight -rhoLeft)))*magSf;
        rhoUFlux = (0.5*(URelTilde*(rhoULeft+rhoURight)-magURelTilde*(rhoURight-rhoULeft))+pTilde*normalVector)*magSf;
        rhoEFlux = (0.5*(URelTilde*(rhoHLeft+rhoHRight)-magURelTilde*(rhoHRight-rhoHLeft)))*magSf;
	forAll(rhoScalarFlux,i)
	{
	    rhoScalarFlux[i] = (0.5*(URelTilde*(rhoScalarLeft[i]+rhoScalarRight[i])-magURelTilde*(rhoScalarRight[i]-rhoScalarLeft[i])))*magSf;
	    //rhoScalarFlux[i] = (0.5*(URelTilde*(rhoScalarLeft[i]+rhoScalarRight[i])-magURelTilde*(rhoRight-rhoLeft)*0.5*(rhoScalarLeft[i]+rhoScalarRight[i])))*magSf;
	}

}

// ************************************************************************* //
