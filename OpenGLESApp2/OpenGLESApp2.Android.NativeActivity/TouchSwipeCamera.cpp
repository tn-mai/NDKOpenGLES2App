#include "TouchSwipeCamera.h"
#include "android_native_app_glue.h"
#include <android/configuration.h>
#include <cmath>

TouchSwipeCamera::TouchSwipeCamera(android_app* a)
{
  sw = 512;
  sh = 512;
  dpFactor = 160.0f / AConfiguration_getDensity(a->config);
  suspending = false;
  Reset();
}

TouchSwipeCamera::~TouchSwipeCamera()
{
}

void TouchSwipeCamera::Reset()
{
  position = Vector3F(2, 2, 0);
  eyeVector = Vector3F(0, 0, 1);
  upVector = Vector3F(0, 1, 0);
  dir = MoveDir_Newtral;
  dragging = false;
}

bool TouchSwipeCamera::HandleEvent(AInputEvent* event)
{
  if (suspending) {
	return false;
  }
  if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
	return false;
  }
  const size_t count = AMotionEvent_getPointerCount(event);
  const int32_t actionAndIndex = AMotionEvent_getAction(event);
  const int32_t action = actionAndIndex & AMOTION_EVENT_ACTION_MASK;
  const int32_t index = (actionAndIndex & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
  bool consumed = false;
  if (count == 1) {
	switch (action) {
	case AMOTION_EVENT_ACTION_DOWN:
	  tapInfo.id = AMotionEvent_getPointerId(event, 0);
	  tapInfo.x = AMotionEvent_getX(event, 0);
	  tapInfo.y = AMotionEvent_getY(event, 0);
	  consumed = true;
	  break;
	case AMOTION_EVENT_ACTION_UP: {
	  const int32_t TAP_TIMEOUT = 180 * 1000000;
	  const int32_t TOUCH_SLOP = 8;
	  const int64_t eventTime = AMotionEvent_getEventTime(event);
	  const int64_t downTime = AMotionEvent_getDownTime(event);
	  if (eventTime - downTime <= TAP_TIMEOUT) {
		if (tapInfo.id == AMotionEvent_getPointerId(event, 0)) {
		  const float x = AMotionEvent_getX(event, 0);
		  const float y = AMotionEvent_getY(event, 0);
		  if ((Vector2F(x, y) - Vector2F(tapInfo.x, tapInfo.y)).Length() < TOUCH_SLOP * dpFactor) {
			MoveDir newDir = MoveDir_Newtral;
			if (y < sh * 0.2f) {
			  newDir = MoveDir_Front2;
			} else if (y < sh * 0.5f) {
			  newDir = MoveDir_Front1;
			} else if (y > sh * 0.7f) {
			  newDir = MoveDir_Back;
			}
			static const MoveDir dirMatrix[4][4] = {
			  /* cur/new  N                1                2                B */
			  /* N */   { MoveDir_Newtral, MoveDir_Front1,  MoveDir_Front2,  MoveDir_Back },
			  /* 1 */   { MoveDir_Newtral, MoveDir_Newtral, MoveDir_Front2,  MoveDir_Back },
			  /* 2 */   { MoveDir_Newtral, MoveDir_Newtral, MoveDir_Newtral, MoveDir_Back },
			  /* B */   { MoveDir_Newtral, MoveDir_Front1,  MoveDir_Front2,  MoveDir_Newtral },
			};
			dir = dirMatrix[dir][newDir];
			consumed = true;
		  }
		}
	  }
	  break;
	}
	}
  }

  {
	switch (action) {
	case AMOTION_EVENT_ACTION_DOWN: {
	  dragInfo.id = AMotionEvent_getPointerId(event, 0);
	  dragStartPoint = Vector2F(AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0));
	  dragCurrentPoint = dragStartPoint;
	  dragStartEyeVector = eyeVector;
	  dragStartUpVector = upVector;
	  dragging = true;
	  consumed = true;
	  return true;
	}
	case AMOTION_EVENT_ACTION_POINTER_DOWN:
	  break;
	case AMOTION_EVENT_ACTION_UP:
	  dragging = false;
	  consumed = true;
	  return true;
	case AMOTION_EVENT_ACTION_POINTER_UP: {
	  const int id = AMotionEvent_getPointerId(event, index);
	  if (dragInfo.id == id) {
		dragging = false;
		consumed = true;
	  }
	  break;
	}
	case AMOTION_EVENT_ACTION_MOVE:
	  dragCurrentPoint = Vector2F(AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0));
	  consumed = true;
	  break;
	}
  }
  return consumed;
}

void TouchSwipeCamera::Update(float /*delta*/)
{
  if (dragging) {
	const float r = std::max(sw, sh);
	Vector2F v2 = dragCurrentPoint - dragStartPoint;
	if (sw < sh) {
	  v2.x *= 2.0f;
	} else {
	  v2.y *= 2.0f;
	}
	const float l2 = v2.Length();
	if (l2 > 0.0f) {
	  v2 /= std::max(l2, r);
	  static const Vector3F vz(0, 0, 1);
	  const Vector3F leftVector = dragStartEyeVector.Cross(dragStartUpVector).Normalize();
	  eyeVector = (Quaternion(dragStartUpVector, v2.x) * Quaternion(leftVector, v2.y)).Apply(dragStartEyeVector).Normalize();
	  upVector = Quaternion(leftVector, v2.y).Apply(dragStartUpVector).Normalize();
	}
  }

  switch (dir) {
  case MoveDir_Front1:
	position += eyeVector * 0.025f;
	break;
  case MoveDir_Front2:
	position += eyeVector * 0.05f;
	break;
  case MoveDir_Back:
	position -= eyeVector * 0.025f;
	break;
  default:
  case MoveDir_Newtral:
	// ‰½‚à‚µ‚È‚¢.
	break;
  }
}

void TouchSwipeCamera::Suspend()
{
  suspending = true;
}

void TouchSwipeCamera::Resume()
{
  suspending = false;
}

void TouchSwipeCamera::Position(const Vector3F& pos)
{
  position = pos;
}

Vector3F TouchSwipeCamera::Position() const
{
  return position;
}

void TouchSwipeCamera::EyeVector(const Vector3F& vec)
{
  eyeVector = vec;
}

Vector3F TouchSwipeCamera::EyeVector() const
{
  return eyeVector;
}

void TouchSwipeCamera::LookAt(const Vector3F& at)
{
  eyeVector = (at - position).Normalize();
}

