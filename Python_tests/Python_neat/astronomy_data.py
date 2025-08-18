from astropy.time import Time
from astropy.coordinates import solar_system_ephemeris, EarthLocation
from astropy.coordinates import get_body
from astropy.coordinates import EarthLocation, AltAz
from astropy import units as u

loc = EarthLocation(lat=33.1424005*u.deg, lon=-96.8599673*u.deg, height=0*u.m)

Objects = ['moon', 'mars', 'jupiter', 'saturn', 'mercury', 'venus', 'sun']  

def get_coords(objectString):
    t = Time.now()
    with solar_system_ephemeris.set('jpl'):
      Object = get_body(objectString, t, loc)

    altazframe = AltAz(obstime=t, location=loc, pressure=0)
    Objectaz=Object.transform_to(altazframe)

    return [Objectaz.alt.degree,Objectaz.az.degree]