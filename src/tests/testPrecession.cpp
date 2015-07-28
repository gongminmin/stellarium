/*
 * Stellarium 
 * Copyright (C) 2015 Georg Zotti
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include <QObject>
#include <QtDebug>
#include <QVariantList>
#include <QtTest>

#include "tests/testPrecession.hpp"
#include "StelUtils.hpp"
#include "VecMath.hpp"

QTEST_MAIN(TestPrecession)

static const double arcSec2Rad=M_PI*2.0/(360.0*3600.0);
static const double eps0=84381.406*arcSec2Rad;

void TestPrecession::initTestCase()
{
}

// example data found in Vondrak et al. 2011: New Precession Expressions, Valid for Long Time Intervals. A&A534, A22.
// Section A.5
// pecl = ( +0.00041724785764001342 -0.40495491104576162693 +0.91433656053126552350 )
//      = ( P_A                     -Q_A*C-Z*S              -Q_A*S+Z*C              )
// pequ = ( -0.29437643797369031532 -0.11719098023370257855 +0.94847708824082091796 )
//      = ( X                        Y                       sqrt(1-x^2-y^2) || 0   )
// This provides only the axes formulation. But the final Precession matrix should then be
// comparable to the matrix that can be reached via Capitaine parametrisation.

void TestPrecession::testPrecessionAnglesVondrak()
{
	const double S=sin(eps0);
	const double C=cos(eps0);
	double Z, W;
	double JulianDay=1219339.078000; // TT
	double epsilon_A, chi_A, omega_A, psi_A;
	double P_A, Q_A, X_A, Y_A;
	double VEC2, VEC3, VEQ3;

	// get angles for Capitaine parameterisation
	getPrecessionAnglesVondrak(JulianDay, &epsilon_A, &chi_A, &omega_A, &psi_A);
	// we have to call this twice with different dates to avoid returning the cached (zero) angles.
	getPrecessionAnglesVondrakPQXYe(-1234.567, &P_A, &Q_A, &X_A, &Y_A, &epsilon_A);
	getPrecessionAnglesVondrakPQXYe(JulianDay, &P_A, &Q_A, &X_A, &Y_A, &epsilon_A);
	Z=sqrt(qMax(1.0-P_A*P_A-Q_A*Q_A, 0.0));
	W=X_A*X_A+Y_A*Y_A;
	VEC2=-Q_A*C -Z*S;
	VEC3=-Q_A*S +Z*C;
	VEQ3=(W<1.0 ? sqrt(1.0-W) : 0.0);

	QVERIFY2(fabs(P_A -0.00041724785764001342)<=0.001, QString("JD %1: Pecl,x: %2 Difference: %3").arg(JulianDay).arg(P_A ).arg(P_A -0.00041724785764001342));
	QVERIFY2(fabs(VEC2+0.40495491104576162693)<=0.001, QString("JD %1: Pecl,y: %2 Difference: %3").arg(JulianDay).arg(VEC2).arg(VEC2+0.40495491104576162693));
	QVERIFY2(fabs(VEC3-0.91433656053126552350)<=0.001, QString("JD %1: Pecl,z: %2 Difference: %3").arg(JulianDay).arg(VEC3).arg(VEC3-0.91433656053126552350));
	QVERIFY2(fabs(X_A +0.29437643797369031532)<=0.001, QString("JD %1: Pequ,x: %2 Difference: %3").arg(JulianDay).arg(X_A ).arg(X_A +0.29437643797369031532));
	QVERIFY2(fabs(Y_A +0.11719098023370257855)<=0.001, QString("JD %1: Pequ,y: %2 Difference: %3").arg(JulianDay).arg(Y_A ).arg(Y_A +0.11719098023370257855));
	QVERIFY2(fabs(VEQ3-0.94847708824082091796)<=0.001, QString("JD %1: Pequ,z: %2 Difference: %3").arg(JulianDay).arg(VEQ3).arg(VEQ3-0.94847708824082091796));

	Vec3d PECL(P_A, VEC2, VEC3);
	Vec3d PEQR(X_A, Y_A, VEQ);
	Vec3d EQX=PEQR^PECL;
	EQX.normalize();
	Vec3d V=PEQR^EQX;
	Mat3d RP(EQX[0], EQX[1], EQX[2], V[0], V[1], V[2], PEQR[0], PEQR[1], PEQR[2]); // result from paper, section A.3
	// Now we create the (hopefully) same rotation matrix from the Capitaine angles.
	Mat4d Rrot=Mat4d::zrotation(chi_A) * Mat4d::xrotation(-omega_A) * Mat4d::zrotation(-psi_A) * Mat4d::xrotation(eps0);
	Mat3d Rcap=Rrot.upper3x3();

	qDebug() << "RP: " << RP.print();
	qDebug() << "Rcap: " << Rcap.print();
}
