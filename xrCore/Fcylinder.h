#pragma once

#ifdef DEBUG
void XRCORE_API __cdecl Msg(LPCSTR format, ...);
#endif

class Fcylinder
{
public:
    Fvector m_center;
    Fvector m_direction;
    float m_height;
    float m_radius;

public:
    IC Fcylinder& invalidate()
    {
        m_center.set(0, 0, 0);
        m_direction.set(0, 0, 0);
        m_height = 0;
        m_radius = 0;
        return *this;
    }

    IC int intersect(const Fvector &start, const Fvector &dir, float afT[2]) const
    {
        float fEpsilon = 1e-12f;

        // set up quadratic Q(t) = a*t^2 + 2*b*t + c
        Fvector kU, kV, kW = m_direction;
        Fvector::generate_orthonormal_basis(kW, kU, kV);
        Fvector kD;
        kD.set(kU.dotproduct(dir), kV.dotproduct(dir), kW.dotproduct(dir));

#ifdef DEBUG
        if (kD.square_magnitude() <= std::numeric_limits<float>::min())
        {
            Msg("dir :%f,%f,%f", dir.x, dir.y, dir.z);
            Msg("kU :%f,%f,%f", kU.x, kU.y, kU.z);
            Msg("kV :%f,%f,%f", kV.x, kV.y, kV.z);
            Msg("kW :%f,%f,%f", kW.x, kW.y, kW.z);
            VERIFY2(0, "KD is zero");
        }
#endif

        float fDLength = kD.normalize_magn();
        float fInvDLength = 1.0f / fDLength;
        Fvector kDiff;
        kDiff.sub(start, m_center);

        Fvector kP;
        kP.set(kU.dotproduct(kDiff), kV.dotproduct(kDiff), kW.dotproduct(kDiff));
        float fHalfHeight = 0.5f * m_height;
        float fRadiusSqr = m_radius * m_radius;

        float fInv, fA, fB, fC, fDiscr, fRoot, fT, fT0, fT1, fTmp0, fTmp1;

        if (_abs(kD.z) >= 1.0f - fEpsilon)
        {
            // line is parallel to cylinder axis
            if (kP.x * kP.x + kP.y * kP.y <= fRadiusSqr)
            {
                fTmp0 = fInvDLength / kD.z;
                afT[0] = (+fHalfHeight - kP.z) * fTmp0;
                afT[1] = (-fHalfHeight - kP.z) * fTmp0;
                return 2;
            }
            else            
                return 0;            
        }

        if (_abs(kD.z) <= fEpsilon)
        {
            // line is perpendicular to axis of cylinder
            if (_abs(kP.z) > fHalfHeight)
            {
                // line is outside the planar caps of cylinder
                return 0;
            }

            fA = kD.x * kD.x + kD.y * kD.y;
            fB = kP.x * kD.x + kP.y * kD.y;
            fC = kP.x * kP.x + kP.y * kP.y - fRadiusSqr;
            fDiscr = fB * fB - fA * fC;

            if (fDiscr < 0.0f)
            {
                // line does not intersect cylinder wall
                return 0;
            }
            else if (fDiscr > 0.0f)
            {
                fRoot = _sqrt(fDiscr);
                fTmp0 = fInvDLength / fA;
                afT[0] = (-fB - fRoot) * fTmp0;
                afT[1] = (-fB + fRoot) * fTmp0;
                return 2;
            }
            else
            {
                afT[0] = -fB * fInvDLength / fA;
                return 1;
            }
        }

        // test plane intersections first
        int iQuantity = 0;
        fInv = 1.0f / kD.z;
        fT0 = (+fHalfHeight - kP.z) * fInv;
        fTmp0 = kP.x + fT0 * kD.x;
        fTmp1 = kP.y + fT0 * kD.y;

        if (fTmp0 * fTmp0 + fTmp1 * fTmp1 <= fRadiusSqr)
            afT[iQuantity++] = fT0 * fInvDLength;

        fT1 = (-fHalfHeight - kP.z) * fInv;
        fTmp0 = kP.x + fT1 * kD.x;
        fTmp1 = kP.y + fT1 * kD.y;

        if (fTmp0 * fTmp0 + fTmp1 * fTmp1 <= fRadiusSqr)
            afT[iQuantity++] = fT1 * fInvDLength;

        if (iQuantity == 2)
        {
            // line intersects both top and bottom
            return 2;
        }

        // If iQuantity == 1, then line must intersect cylinder wall
        // somewhere between caps in a single point.  This case is detected
        // in the following code that tests for intersection between line and
        // cylinder wall.

        fA = kD.x * kD.x + kD.y * kD.y;
        fB = kP.x * kD.x + kP.y * kD.y;
        fC = kP.x * kP.x + kP.y * kP.y - fRadiusSqr;
        fDiscr = fB * fB - fA * fC;

        if (fDiscr < 0.0f)
        {
            // line does not intersect cylinder wall
            //VERIFY( iQuantity == 0 );
            return 0;
        }
        else if (fDiscr > 0.0f)
        {
            fRoot = _sqrt(fDiscr);
            fInv = 1.0f / fA;
            fT = (-fB - fRoot) * fInv;

            if (fT0 <= fT1)
            {
                if (fT0 <= fT && fT <= fT1)
                    afT[iQuantity++] = fT * fInvDLength;
            }
            else
            {
                if (fT1 <= fT && fT <= fT0)
                    afT[iQuantity++] = fT * fInvDLength;
            }

            if (iQuantity == 2)
            {
                // Line intersects one of top/bottom of cylinder and once on
                // cylinder wall.
                return 2;
            }

            fT = (-fB + fRoot) * fInv;

            if (fT0 <= fT1)
            {
                if (fT0 <= fT && fT <= fT1)
                    afT[iQuantity++] = fT * fInvDLength;
            }
            else
            {
                if (fT1 <= fT && fT <= fT0)
                    afT[iQuantity++] = fT * fInvDLength;
            }
        }
        else
        {
            fT = -fB / fA;
            if (fT0 <= fT1)
            {
                if (fT0 <= fT && fT <= fT1)
                    afT[iQuantity++] = fT * fInvDLength;
            }
            else
            {
                if (fT1 <= fT && fT <= fT0)
                    afT[iQuantity++] = fT * fInvDLength;
            }
        }

        return iQuantity;
    }

    enum ERP_Result
    {
        rpNone = 0,
        rpOriginInside = 1,
        rpOriginOutside = 2,
        fcv_forcedword = u32(-1)
    };

    IC ERP_Result intersect(const Fvector &start, const Fvector &dir, float &dist) const
    {
        float afT[2];
        int cnt;

        if (0 != (cnt = intersect(start, dir, afT)))
        {
            bool o_inside = false;
            bool b_result = false;

            for (int k = 0; k < cnt; k++)
            {
                if (afT[k] < 0.f)
                {
                    if (cnt == 2)
                        o_inside = true;
                    continue;
                }

                if (afT[k] < dist)
                {
                    dist = afT[k];
                    b_result = true;
                }
            }

            return b_result ? (o_inside ? rpOriginInside : rpOriginOutside) : rpNone;
        }
        else        
            return rpNone;        
    }
};

ICF bool _valid(const Fcylinder &c) { return _valid(c.m_center) && _valid(c.m_direction) && _valid(c.m_height) && _valid(c.m_height); }
