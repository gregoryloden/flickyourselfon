#include "EntityState.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/GameState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"

//////////////////////////////// EntityState ////////////////////////////////
GLuint EntityState::postZoomFrameBufferId = 0;
GLuint EntityState::postZoomRenderBufferId = 0;
GLuint EntityState::preZoomFrameBufferId = 0;
GLuint EntityState::preZoomTextureId = 0;
EntityState::EntityState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, x(newConstantValue(0.0f))
, renderInterpolatedX(true)
, y(newConstantValue(0.0f))
, renderInterpolatedY(true)
, entityAnimation(nullptr)
, zoom(newConstantValue(1.0f))
, lastUpdateTicksTime(0) {
}
EntityState::~EntityState() {}
void EntityState::copyEntityState(EntityState* other) {
	x.set(other->x.get());
	renderInterpolatedX = other->renderInterpolatedX;
	y.set(other->y.get());
	renderInterpolatedY = other->renderInterpolatedY;
	entityAnimation.set(other->entityAnimation.get());
	lastUpdateTicksTime = other->lastUpdateTicksTime;
}
void EntityState::setupZoomFrameBuffers() {
	//set up the post-zoom frame buffer, which uses a render buffer
	glGenFramebuffers(1, &postZoomFrameBufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, postZoomFrameBufferId);
	glGenRenderbuffers(1, &postZoomRenderBufferId);
	glBindRenderbuffer(GL_RENDERBUFFER, postZoomRenderBufferId);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, (GLsizei)Config::windowScreenWidth, (GLsizei)Config::windowScreenHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, postZoomRenderBufferId);
	#ifdef DEBUG
		Opengl::checkAndLogFrameBufferStatus(GL_FRAMEBUFFER, "postZoomFrameBuffer");
	#endif

	//set up the pre-zoom frame buffer, which uses a texture
	glGenFramebuffers(1, &preZoomFrameBufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, preZoomFrameBufferId);
	glGenTextures(1, &preZoomTextureId);
	glBindTexture(GL_TEXTURE_2D, preZoomTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		//image format + size
		GL_RGB, (GLsizei)Config::windowScreenWidth, (GLsizei)Config::windowScreenHeight,
		0,
		//image source format + data
		GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, preZoomTextureId, 0);
	#ifdef DEBUG
		Opengl::checkAndLogFrameBufferStatus(GL_FRAMEBUFFER, "preZoomFrameBuffer");
	#endif

	//return to the default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
float EntityState::getRenderCenterWorldX(int ticksTime) {
	return x.get()->getValue(renderInterpolatedX ? ticksTime - lastUpdateTicksTime : 0);
}
float EntityState::getRenderCenterWorldY(int ticksTime) {
	return y.get()->getValue(renderInterpolatedY ? ticksTime - lastUpdateTicksTime : 0);
}
float EntityState::getRenderCenterScreenX(EntityState* camera, int ticksTime) {
	return getRenderCenterScreenXFromWorldX(getRenderCenterWorldX(ticksTime), camera, ticksTime);
}
float EntityState::getRenderCenterScreenXFromWorldX(float worldX, EntityState* camera, int ticksTime) {
	//convert these to ints first to align with the map in case the camera is not this entity
	int screenXOffset = (int)worldX - (int)camera->getRenderCenterWorldX(ticksTime);
	return (float)screenXOffset + (float)Config::gameScreenWidth * 0.5f;
}
float EntityState::getRenderCenterScreenY(EntityState* camera, int ticksTime) {
	return getRenderCenterScreenYFromWorldY(getRenderCenterWorldY(ticksTime), camera, ticksTime);
}
float EntityState::getRenderCenterScreenYFromWorldY(float worldY, EntityState* camera, int ticksTime) {
	//convert these to ints first to align with the map in case the camera is not this entity
	int screenYOffset = (int)worldY - (int)camera->getRenderCenterWorldY(ticksTime);
	return (float)screenYOffset + (float)Config::gameScreenHeight * 0.5f;
}
int EntityState::getAnimationTicksDuration() {
	return entityAnimation.get() != nullptr ? entityAnimation.get()->getTotalTicksDuration() : 0;
}
SpriteDirection EntityState::getSpriteDirection(float pX, float pY) {
	return abs(pY) >= abs(pX)
		? pY >= 0 ? SpriteDirection::Down : SpriteDirection::Up
		: pX >= 0 ? SpriteDirection::Right : SpriteDirection::Left;
}
SpriteDirection EntityState::getOppositeDirection(SpriteDirection spriteDirection) {
	switch (spriteDirection) {
		case SpriteDirection::Right: return SpriteDirection::Left;
		case SpriteDirection::Up: return SpriteDirection::Down;
		case SpriteDirection::Left: return SpriteDirection::Right;
		default: return SpriteDirection::Up;
	}
}
void EntityState::setPosition(float pX, float pY, int pLastUpdateTicksTime) {
	//get our original position
	float originalXPosition = x.get()->getValue(0);
	float originalYPosition = y.get()->getValue(0);
	//find how far we have to go
	int timediff = pLastUpdateTicksTime - lastUpdateTicksTime;
	float addX = pX - x.get()->getValue(timediff);
	float addY = pY - y.get()->getValue(timediff);
	//set a new position offset by how far we have to move
	x.set(x.get()->copyWithConstantValue(originalXPosition + addX));
	y.set(y.get()->copyWithConstantValue(originalYPosition + addY));
}
void EntityState::setVelocity(DynamicValue* vx, DynamicValue* vy, int pLastUpdateTicksTime) {
	int timediff = pLastUpdateTicksTime - lastUpdateTicksTime;
	x.set(vx->copyWithConstantValue(x.get()->getValue(timediff)));
	y.set(vy->copyWithConstantValue(y.get()->getValue(timediff)));
	lastUpdateTicksTime = pLastUpdateTicksTime;
}
void EntityState::beginEntityAnimation(
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components, int ticksTime)
{
	entityAnimation.set(newEntityAnimation(ticksTime, components));
	renderInterpolatedX = true;
	renderInterpolatedY = true;
	//update it once to get it started
	entityAnimation.get()->update(this, ticksTime);
}
float EntityState::renderBeginZoom(int ticksTime) {
	float zoomValue = zoom.get()->getValue(ticksTime - lastUpdateTicksTime);
	if (zoomValue == 1)
		return 1;
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, preZoomFrameBufferId);
	Opengl::orientRenderTarget(false);
	glViewport(0, 0, (GLsizei)Config::windowScreenWidth, (GLsizei)Config::windowScreenHeight);
	return zoomValue;
}
void EntityState::renderEndZoom(float zoomValue) {
	//render the image zoomed
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, postZoomFrameBufferId);
	Opengl::orientRenderTarget(true);
	float topLeftBorder = (zoomValue - 1.0f) / zoomValue * 0.5f;
	float bottomRightBorder = 1.0f - topLeftBorder;
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, preZoomTextureId);
	glBegin(GL_QUADS);
	glTexCoord2f(topLeftBorder, topLeftBorder);
	glVertex2i(0, 0);
	glTexCoord2f(bottomRightBorder, topLeftBorder);
	glVertex2i((GLint)Config::windowScreenWidth, 0);
	glTexCoord2f(bottomRightBorder, bottomRightBorder);
	glVertex2i((GLint)Config::windowScreenWidth, (GLint)Config::windowScreenHeight);
	glTexCoord2f(topLeftBorder, bottomRightBorder);
	glVertex2i(0, (GLint)Config::windowScreenHeight);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	//render the zoomed image to the screen
	glBindFramebuffer(GL_READ_FRAMEBUFFER, postZoomFrameBufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, (GLint)Config::windowDisplayWidth, (GLint)Config::windowDisplayHeight);
	glBlitFramebuffer(
		//source
		0, 0, (GLint)Config::windowScreenWidth, (GLint)Config::windowScreenHeight,
		//destination
		0, 0, (GLint)Config::windowDisplayWidth, (GLint)Config::windowDisplayHeight,
		GL_COLOR_BUFFER_BIT,
		GL_NEAREST);
}

//////////////////////////////// DynamicCameraAnchor ////////////////////////////////
DynamicCameraAnchor::DynamicCameraAnchor(objCounterParameters())
: EntityState(objCounterArguments())
, screenOverlayR(nullptr)
, screenOverlayG(nullptr)
, screenOverlayB(nullptr)
, screenOverlayA(nullptr)
, shouldSwitchToPlayerCamera(false) {
}
DynamicCameraAnchor::~DynamicCameraAnchor() {}
DynamicCameraAnchor* DynamicCameraAnchor::produce(objCounterParameters()) {
	initializeWithNewFromPool(d, DynamicCameraAnchor)
	d->screenOverlayR.set(newConstantValue(0.0f));
	d->screenOverlayG.set(newConstantValue(0.0f));
	d->screenOverlayB.set(newConstantValue(0.0f));
	d->screenOverlayA.set(newConstantValue(0.0f));
	d->shouldSwitchToPlayerCamera = false;
	return d;
}
void DynamicCameraAnchor::copyDynamicCameraAnchor(DynamicCameraAnchor* other) {
	copyEntityState(other);
	screenOverlayR.set(other->screenOverlayR.get());
	screenOverlayG.set(other->screenOverlayG.get());
	screenOverlayB.set(other->screenOverlayB.get());
	screenOverlayA.set(other->screenOverlayA.get());
}
pooledReferenceCounterDefineRelease(DynamicCameraAnchor)
void DynamicCameraAnchor::updateWithPreviousDynamicCameraAnchor(
	DynamicCameraAnchor* prev, bool hasKeyboardControl, int ticksTime)
{
	copyDynamicCameraAnchor(prev);
	if (entityAnimation.get() != nullptr) {
		if (entityAnimation.get()->update(this, ticksTime))
			return;
		entityAnimation.set(nullptr);
	} else if (hasKeyboardControl) {
		static constexpr float speedPerSecond = 120.0f;
		static constexpr float diagonalSpeedPerSecond = speedPerSecond * (float)MathUtils::sqrtConst(0.5);
		static constexpr float sprintModifier = 2.0f;
		const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
		char xDirection = (char)(keyboardState[Config::rightKeyBinding.value] - keyboardState[Config::leftKeyBinding.value]);
		char yDirection = (char)(keyboardState[Config::downKeyBinding.value] - keyboardState[Config::upKeyBinding.value]);
		float speedPerTick =
			((xDirection & yDirection) != 0 ? diagonalSpeedPerSecond : speedPerSecond) / (float)Config::ticksPerSecond;
		if (keyboardState[Config::sprintKeyBinding.value] != 0)
			speedPerTick *= sprintModifier;
		int ticksSinceLastUpdate = ticksTime - lastUpdateTicksTime;
		float newX = x.get()->getValue(ticksSinceLastUpdate);
		float newY = y.get()->getValue(ticksSinceLastUpdate);
		x.set(newCompositeQuarticValue(newX, (float)xDirection * speedPerTick, 0.0f, 0.0f, 0.0f));
		y.set(newCompositeQuarticValue(newY, (float)yDirection * speedPerTick, 0.0f, 0.0f, 0.0f));
		lastUpdateTicksTime = ticksTime;
	}
}
void DynamicCameraAnchor::setScreenOverlayColor(
	DynamicValue* r, DynamicValue* g, DynamicValue* b, DynamicValue* a, int pLastUpdateTicksTime)
{
	screenOverlayR.set(r);
	screenOverlayG.set(g);
	screenOverlayB.set(b);
	screenOverlayA.set(a);
	lastUpdateTicksTime = pLastUpdateTicksTime;
}
void DynamicCameraAnchor::setNextCamera(GameState* nextGameState, int ticksTime) {
	if (shouldSwitchToPlayerCamera)
		nextGameState->setPlayerCamera();
	else
		nextGameState->setDynamicCamera();
}
void DynamicCameraAnchor::render(int ticksTime) {
	int timediff = ticksTime - lastUpdateTicksTime;
	GLfloat r = (GLfloat)screenOverlayR.get()->getValue(timediff);
	GLfloat g = (GLfloat)screenOverlayG.get()->getValue(timediff);
	GLfloat b = (GLfloat)screenOverlayB.get()->getValue(timediff);
	GLfloat a = (GLfloat)screenOverlayA.get()->getValue(timediff);
	if (a > 0)
		SpriteSheet::renderFilledRectangle(r, g, b, a, 0, 0, (GLint)Config::gameScreenWidth, (GLint)Config::gameScreenHeight);
	if (!hasAnimation()) {
		static constexpr int borderInset = 3;
		int xIncrement = (Config::gameScreenWidth - SpriteRegistry::borderArrows->getSpriteWidth()) / 2 - borderInset;
		int yIncrement = (Config::gameScreenHeight - SpriteRegistry::borderArrows->getSpriteHeight()) / 2 - borderInset;
		for (int arrowYI = 0; arrowYI <= 2; arrowYI++) {
			GLint arrowY = (GLint)(borderInset + arrowYI * yIncrement);
			for (int arrowXI = 0; arrowXI <= 2; arrowXI++) {
				if (arrowXI == 1 && arrowYI == 1)
					continue;
				GLint arrowX = (GLint)(borderInset + arrowXI * xIncrement);
				(SpriteRegistry::borderArrows->*SpriteSheet::renderSpriteAtScreenPosition)(arrowXI, arrowYI, arrowX, arrowY);
			}
		}
	}
}

//////////////////////////////// Particle ////////////////////////////////
Particle::Particle(objCounterParameters())
: EntityState(objCounterArguments())
, spriteAnimation(nullptr)
, spriteAnimationStartTicksTime(0)
, spriteDirection(SpriteDirection::Right)
, r(0)
, g(0)
, b(0)
, isAbovePlayer(false) {
}
Particle::~Particle() {
	//don't delete the sprite animation, SpriteRegistry owns it
}
Particle* Particle::produce(objCounterParametersComma() float pX, float pY, float pR, float pG, float pB, bool pIsAbovePlayer) {
	initializeWithNewFromPool(p, Particle)
	p->x.set(newConstantValue(pX));
	p->y.set(newConstantValue(pY));
	p->spriteAnimation = nullptr;
	p->r = pR;
	p->g = pG;
	p->b = pB;
	p->isAbovePlayer = pIsAbovePlayer;
	return p;
}
void Particle::copyParticle(Particle* other) {
	copyEntityState(other);
	setSpriteAnimation(other->spriteAnimation, other->spriteAnimationStartTicksTime);
	setDirection(other->spriteDirection);
	r = other->r;
	g = other->g;
	b = other->b;
	isAbovePlayer = other->isAbovePlayer;
}
pooledReferenceCounterDefineRelease(Particle)
void Particle::setSpriteAnimation(SpriteAnimation* pSpriteAnimation, int pSpriteAnimationStartTicksTime) {
	spriteAnimation = pSpriteAnimation;
	spriteAnimationStartTicksTime = pSpriteAnimationStartTicksTime;
}
void Particle::setDirection(SpriteDirection pSpriteDirection) {
	spriteDirection = pSpriteDirection;
}
bool Particle::updateWithPreviousParticle(Particle* prev, int ticksTime) {
	copyParticle(prev);
	//if we have an entity animation, update with that instead
	if (prev->entityAnimation.get() != nullptr) {
		if (entityAnimation.get()->update(this, ticksTime))
			return true;
		entityAnimation.set(nullptr);
	}
	return false;
}
void Particle::render(EntityState* camera, int ticksTime) {
	if (spriteAnimation == nullptr)
		return;

	(spriteAnimation->getSprite()->*SpriteSheet::setSpriteColor)((GLfloat)r, (GLfloat)g, (GLfloat)b, 1.0f);
	float renderCenterX = getRenderCenterScreenX(camera,  ticksTime);
	float renderCenterY = getRenderCenterScreenY(camera,  ticksTime);
	spriteAnimation->renderUsingCenter(
		renderCenterX, renderCenterY, ticksTime - spriteAnimationStartTicksTime, 0, (int)spriteDirection);
	(spriteAnimation->getSprite()->*SpriteSheet::setSpriteColor)(1.0f, 1.0f, 1.0f, 1.0f);
}
