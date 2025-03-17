
//-----------------------------------------------------------------------------
void EmitterDefData::_cloneFrom( const EmitterDefData *_original )
{
    OGRE_ASSERT_HIGH( dynamic_cast<const EmitterDefData *>( _original ) );

    const EmitterDefData *original = static_cast<const EmitterDefData *>( _original );
    this->mDimensions = original->mDimensions;
    this->mParent = original->mParent;
    this->mPosition = original->mPosition;
    this->mEmissionRate = original->mEmissionRate;
    this->mDirection = original->mDirection;
    this->mUp = original->mUp;
    this->mUseDirPositionRef = original->mUseDirPositionRef;
    this->mDirPositionRef = original->mDirPositionRef;
    this->mAngle = original->mAngle;
    this->mMinSpeed = original->mMinSpeed;
    this->mMaxSpeed = original->mMaxSpeed;
    this->mMinTTL = original->mMinTTL;
    this->mMaxTTL = original->mMaxTTL;
    this->mColourRangeStart = original->mColourRangeStart;
    this->mColourRangeEnd = original->mColourRangeEnd;
    this->mEnabled = original->mEnabled;
    this->mStartTime = original->mStartTime;
    this->mDurationMin = original->mDurationMin;
    this->mDurationMax = original->mDurationMax;
    this->mDurationRemain = original->mDurationRemain;
    this->mRepeatDelayMin = original->mRepeatDelayMin;
    this->mRepeatDelayMax = original->mRepeatDelayMax;
    this->mRepeatDelayRemain = original->mRepeatDelayRemain;
    this->mRemainder = original->mRemainder;
    this->mName = original->mName;
    this->mEmittedEmitter = original->mEmittedEmitter;
    this->mEmitted = original->mEmitted;
}
