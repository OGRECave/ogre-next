
//-----------------------------------------------------------------------------
void EmitterDefData::_cloneFrom( const EmitterDefData *_original )
{
    OGRE_ASSERT_HIGH( dynamic_cast<const EmitterDefData *>( _original ) );

    const EmitterDefData *original = static_cast<const EmitterDefData *>( _original );
    this->mDimensions = original->mDimensions;
    this->mParent = original->mParent;
    this->mEmissionRate = original->mEmissionRate;
    this->mUseDirPositionRef = original->mUseDirPositionRef;
    this->mMinSpeed = original->mMinSpeed;
    this->mMaxSpeed = original->mMaxSpeed;
    this->mMinTTL = original->mMinTTL;
    this->mMaxTTL = original->mMaxTTL;
    this->mEnabled = original->mEnabled;
    this->mStartTime = original->mStartTime;
    this->mDurationMin = original->mDurationMin;
    this->mDurationMax = original->mDurationMax;
    this->mDurationRemain = original->mDurationRemain;
    this->mRepeatDelayMin = original->mRepeatDelayMin;
    this->mRepeatDelayMax = original->mRepeatDelayMax;
    this->mRepeatDelayRemain = original->mRepeatDelayRemain;
    this->mRemainder = original->mRemainder;
    this->mEmitted = original->mEmitted;
    this->mPosition = original->mPosition;
    this->mDirection = original->mDirection;
    this->mUp = original->mUp;
    this->mDirPositionRef = original->mDirPositionRef;
    this->mAngle = original->mAngle;
    this->mColourRangeStart = original->mColourRangeStart;
    this->mColourRangeEnd = original->mColourRangeEnd;
    this->mName = original->mName;
    this->mEmittedEmitter = original->mEmittedEmitter;
}
