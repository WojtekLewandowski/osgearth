/* -*-c++-*- */
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
* Copyright 2008-2010 Pelican Mapping
* http://osgearth.org
*
* osgEarth is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
#include <osgEarthUtil/TerrainProfile>
#include <osgEarth/GeoMath>

using namespace osgEarth;
using namespace osgEarth::Util;

/***************************************************/
TerrainProfile::TerrainProfile():
_spacing( 1.0 )
{
}

TerrainProfile::TerrainProfile(const TerrainProfile& rhs):
_spacing( rhs._spacing ),
_elevations( rhs._elevations )
{
}

void
TerrainProfile::clear()
{
    _elevations.clear();
}

double
TerrainProfile::getSpacing() const
{
    return _spacing;
}

void
TerrainProfile::setSpacing( double spacing )
{
    _spacing = spacing;
}

void
TerrainProfile::addElevation( double elevation )
{
    _elevations.push_back( elevation );
}

double
TerrainProfile::getElevation( int i ) const
{
    if (i >= 0 && i < static_cast<int>(_elevations.size())) return _elevations[i];
    return DBL_MAX;
}

double
TerrainProfile::getDistance( int i ) const
{
    return (double)i * _spacing;
}

double
TerrainProfile::getTotalDistance() const
{
    return getDistance( getNumElevations()-1 );
}

unsigned int
TerrainProfile::getNumElevations() const
{
    return _elevations.size();
}

void
TerrainProfile::getElevationRanges(double &min, double &max )
{
    min = DBL_MAX;
    max = -DBL_MAX;

    for (unsigned int i = 0; i < _elevations.size(); i++)
    {
        if (_elevations[i] < min) min = _elevations[i];
        if (_elevations[i] > max) max = _elevations[i];
    }
}

/***************************************************/
TerrainProfileCalculator::TerrainProfileCalculator(MapNode* mapNode, const GeoPoint& start, const GeoPoint& end):
_mapNode( mapNode ),
_start( start),
_end( end ),
_numSamples( 100 )
{        
    _mapNode->getTerrain()->addTerrainCallback( this );        
    recompute();
}

TerrainProfileCalculator::~TerrainProfileCalculator()
{
    _mapNode->getTerrain()->removeTerrainCallback( this );
}

void TerrainProfileCalculator::addChangedCallback( ChangedCallback* callback )
{
    _changedCallbacks.push_back( callback );
}

void TerrainProfileCalculator::removeChangedCallback( ChangedCallback* callback )
{
    ChangedCallbackList::iterator i = std::find( _changedCallbacks.begin(), _changedCallbacks.end(), callback);
    if (i != _changedCallbacks.end())
    {
        _changedCallbacks.erase( i );
    }    
}

const TerrainProfile& TerrainProfileCalculator::getProfile() const
{
    return _profile;
}

unsigned int TerrainProfileCalculator::getNumSamples() const
{
    return _numSamples;
}

void TerrainProfileCalculator::setNumSamples( unsigned int numSamples)
{
    if (_numSamples != numSamples)
    {
        _numSamples = numSamples;
        recompute();
    }
}

const GeoPoint& TerrainProfileCalculator::getStart() const
{
    return _start;
}

const GeoPoint& TerrainProfileCalculator::getEnd() const
{
    return _end;
}

void TerrainProfileCalculator::setStartEnd(const GeoPoint& start, const GeoPoint& end)
{
    if (_start != start || _end != end)
    {
        _start = start;
        _end = end;
        recompute();
    }
}

void TerrainProfileCalculator::onTileAdded(const osgEarth::TileKey& tileKey, osg::Node* terrain, TerrainCallbackContext&)
{
    GeoExtent extent( _start.getSRS());
    extent.expandToInclude(_start.x(), _start.y());
    extent.expandToInclude(_end.x(), _end.y());

    if (tileKey.getExtent().intersects( extent ))
    {
        recompute();
    }
}

void TerrainProfileCalculator::recompute()
{
    computeTerrainProfile( _mapNode.get(), _start, _end, _numSamples, _profile);
    for( ChangedCallbackList::iterator i = _changedCallbacks.begin(); i != _changedCallbacks.end(); i++ )
    {
        i->get()->onChanged(this);
    }	
}

void TerrainProfileCalculator::computeTerrainProfile( osgEarth::MapNode* mapNode, const GeoPoint& start, const GeoPoint& end, unsigned int numSamples, TerrainProfile& profile)
{
    GeoPoint geoStart(start);
    geoStart.makeGeographic();

    GeoPoint geoEnd(end);
    geoEnd.makeGeographic();

    double startXRad = osg::DegreesToRadians( geoStart.x() );
    double startYRad = osg::DegreesToRadians( geoStart.y() );
    double endXRad = osg::DegreesToRadians( geoEnd.x() );
    double endYRad = osg::DegreesToRadians( geoEnd.y() );

    double distance = osgEarth::GeoMath::distance(startYRad, startXRad, endYRad, endXRad );

    double spacing = distance / ((double)numSamples - 1.0);

    profile.setSpacing( spacing );
    profile.clear();

    for (unsigned int i = 0; i < numSamples; i++)
    {
        double t = (double)i / (double)numSamples;
        double lat, lon;
        GeoMath::interpolate( startYRad, startXRad, endYRad, endXRad, t, lat, lon );
        double hamsl;
        mapNode->getTerrain()->getHeight( osg::RadiansToDegrees(lon), osg::RadiansToDegrees(lat), &hamsl );
        profile.addElevation( hamsl );
    }
}