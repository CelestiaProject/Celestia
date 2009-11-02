// rotation.h
//
// Copyright (C) 2004-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_ROTATION_H_
#define _CELENGINE_ROTATION_H_

#include <Eigen/Geometry>


/*! A RotationModel object describes the orientation of an object
 *  over some time range.
 */
class RotationModel
{
 public:
    virtual ~RotationModel() {};

    /*! Return the orientation of an object in its reference frame at the
     * specified time (TDB). Some rotations can be decomposed into two parts:
     * a fixed or slowly varying part, and a much more rapidly varying part.
     * The rotation of a planet is such an example. The rapidly varying part
     * is referred to as spin; the slowly varying part determines the
     * equatorial plane. When the rotation of an object can be decomposed
     * in this way, the overall orientation = spin * equator. Otherwise,
     * orientation = spin.
     */
    Eigen::Quaterniond orientationAtTime(double tjd) const
    {
        return spin(tjd) * equatorOrientationAtTime(tjd);
    }

    virtual Eigen::Vector3d angularVelocityAtTime(double tjd) const;

    /*! Return the orientation of the equatorial plane (normal to the primary
     *  axis of rotation.) The overall orientation of the object is
     *  spin * equator. If there is no primary axis of rotation, equator = 1
     *  and orientation = spin.
     */
    virtual Eigen::Quaterniond equatorOrientationAtTime(double /*tjd*/) const
    {
        return Eigen::Quaterniond::Identity();
    }

    /*! Return the rotation about primary axis of rotation (if there is one.)
     *  The overall orientation is spin * equator. For objects without a
     *  primary axis of rotation, spin *is* the orientation.
     */
    virtual Eigen::Quaterniond spin(double tjd) const = 0;

    virtual double getPeriod() const
    {
        return 0.0;
    };

    virtual bool isPeriodic() const
    {
        return false;
    };

    // Return the time range over which the orientation model is valid;
    // if the model is always valid, begin and end should be equal.
    virtual void getValidRange(double& begin, double& end) const
    {
        begin = 0.0;
        end = 0.0;
    };
};


/*! CachingRotationModel is an abstract base class for complicated rotation
 *  models that are computationally expensive. The last calculated spin,
 *  equator orientation, and angular velocity are all cached and reused in
 *  order to avoid redundant calculation. Subclasses must override computeSpin(),
 *  computeEquatorOrientation(), and getPeriod(). The default implementation
 *  of computeAngularVelocity uses differentiation to approximate the
 *  the instantaneous angular velocity. It may be overridden if there is some
 *  better means to calculate the angular velocity for a specific rotation
 *  model.
 */
class CachingRotationModel : public RotationModel
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    CachingRotationModel();
    virtual ~CachingRotationModel();
    
    Eigen::Quaterniond spin(double tjd) const;
    Eigen::Quaterniond equatorOrientationAtTime(double tjd) const;
    Eigen::Vector3d angularVelocityAtTime(double tjd) const;
    
    virtual Eigen::Quaterniond computeEquatorOrientation(double tjd) const = 0;
    virtual Eigen::Quaterniond computeSpin(double tjd) const = 0;
    virtual Eigen::Vector3d computeAngularVelocity(double tjd) const;
    virtual double getPeriod() const = 0;
    virtual bool isPeriodic() const = 0;
    
private:
    mutable Eigen::Quaterniond lastSpin;
    mutable Eigen::Quaterniond lastEquator;
    mutable Eigen::Vector3d lastAngularVelocity;
    mutable double lastTime;
    mutable bool spinCacheValid;
    mutable bool equatorCacheValid;
    mutable bool angularVelocityCacheValid;
};


/*! The simplest rotation model is ConstantOrientation, which describes
 *  an orientation that is fixed within a reference frame.
 */
class ConstantOrientation : public RotationModel
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    ConstantOrientation(const Eigen::Quaterniond& q);
    virtual ~ConstantOrientation();

    virtual Eigen::Quaterniond spin(double tjd) const;
    virtual Eigen::Vector3d angularVelocityAtTime(double tjd) const;

 private:
    Eigen::Quaterniond orientation;
};


/*! UniformRotationModel describes an object that rotates with a constant
 *  angular velocity.
 */
class UniformRotationModel : public RotationModel
{
 public:
    UniformRotationModel(double _period,
                         float _offset,
                         double _epoch,
                         float _inclination,
                         float _ascendingNode);
    virtual ~UniformRotationModel();

    virtual bool isPeriodic() const;
    virtual double getPeriod() const;
    virtual Eigen::Quaterniond equatorOrientationAtTime(double tjd) const;
    virtual Eigen::Quaterniond spin(double tjd) const;
    virtual Eigen::Vector3d angularVelocityAtTime(double tjd) const;

 private:
    double period;       // sidereal rotation period
    float offset;        // rotation at epoch
    double epoch;
    float inclination;   // tilt of rotation axis w.r.t. reference plane
    float ascendingNode; // longitude of ascending node of equator on the refernce plane
};


/*! PrecessingRotationModel describes an object with a spin axis that
 *  precesses at a constant rate about some axis.
 */
class PrecessingRotationModel : public RotationModel
{
 public:
    PrecessingRotationModel(double _period,
                            float _offset,
                            double _epoch,
                            float _inclination,
                            float _ascendingNode,
                            double _precPeriod);
    virtual ~PrecessingRotationModel();

    virtual bool isPeriodic() const;
    virtual double getPeriod() const;
    virtual Eigen::Quaterniond equatorOrientationAtTime(double tjd) const;
    virtual Eigen::Quaterniond spin(double tjd) const;

 private:
    double period;       // sidereal rotation period (in Julian days)
    float offset;        // rotation at epoch
    double epoch;
    float inclination;   // tilt of rotation axis w.r.t. reference plane
    float ascendingNode; // longitude of ascending node of equator on the refernce plane

    double precessionPeriod; // period of precession (in Julian days)
};

#endif // _CELENGINE_ROTATION_H_
