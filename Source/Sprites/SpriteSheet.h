#include "General/General.h"

#define newSpriteSheet(imageSurface, horizontalSpriteCount, verticalSpriteCount, hasBottomRightPixelBorder) \
	newWithArgs(SpriteSheet, imageSurface, horizontalSpriteCount, verticalSpriteCount, hasBottomRightPixelBorder)
#define newSpriteSheetWithImagePath(imagePath, horizontalSpriteCount, verticalSpriteCount, hasBottomRightPixelBorder) \
	produceWithArgs(SpriteSheet, imagePath, horizontalSpriteCount, verticalSpriteCount, hasBottomRightPixelBorder)

class SpriteSheet onlyInDebug(: public ObjCounter) {
private:
	static SDL_Renderer* activeRectangleRenderer;

	GLuint textureId;
	SDL_Surface* renderSurface;
	SDL_Renderer* activeRenderer;
	SDL_Texture* activeRenderTexture;
	int spriteWidth;
	int spriteHeight;
	float spriteTexPixelWidth;
	float spriteTexPixelHeight;
	float centerAnchorX;
	float centerAnchorY;

public:
	SpriteSheet(
		objCounterParametersComma()
		SDL_Surface* imageSurface,
		int horizontalSpriteCount,
		int verticalSpriteCount,
		bool hasBottomRightPixelBorder);
	virtual ~SpriteSheet();

	int getSpriteWidth() { return spriteWidth; }
	int getSpriteHeight() { return spriteHeight; }
	void setBottomAnchorY() { centerAnchorY = (float)spriteHeight; }
	//load the surface at the image path and then build a SpriteSheet
	static SpriteSheet* produce(
		objCounterParametersComma()
		const char* imagePath,
		int horizontalSpriteCount,
		int verticalSpriteCount,
		bool hasBottomRightPixelBorder);
	//render using OpenGL rendering functions
	static void renderWithOpenGL();
	//render using SDL_Renderer/SDL_Texture rendering functions
	static void renderWithRenderer(SDL_Renderer* rectangleRenderer);
	//load a render texture for the given renderer, writing the old renderer and texture to the given out parameters
	void loadRenderTexture(SDL_Renderer* renderer, SDL_Renderer** outOldRenderer, SDL_Texture** outOldTexture);
	//destroy the texture on this SpriteSheet and restore it with the given texture and renderer
	void unloadRenderTexture(SDL_Renderer* renderer, SDL_Texture* texture);
	//load a texture with this SpriteSheet's surface, use it for rendering, and then delete it
	void withRenderTexture(SDL_Renderer* renderer, function<void()> renderWithTexture);
	//set a color to render with
	static void (SpriteSheet::* setSpriteColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	//set a color to render with using OpenGL
	void setSpriteColorOpenGL(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	//set a color to render with using the renderer
	void setSpriteColorRenderer(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	//draw the specified region of the sprite sheet
	static void (SpriteSheet::* renderSpriteSheetRegionAtScreenRegion)(
		int spriteLeftX,
		int spriteTopY,
		int spriteRightX,
		int spriteBottomY,
		GLint drawLeftX,
		GLint drawTopY,
		GLint drawRightX,
		GLint drawBottomY);
	//draw the specified region of the sprite sheet to the screen
	void renderSpriteSheetRegionAtScreenRegionOpenGL(
		int spriteLeftX,
		int spriteTopY,
		int spriteRightX,
		int spriteBottomY,
		GLint drawLeftX,
		GLint drawTopY,
		GLint drawRightX,
		GLint drawBottomY);
	//draw the specified region of the sprite sheet to the renderer
	void renderSpriteSheetRegionAtScreenRegionRenderer(
		int spriteLeftX,
		int spriteTopY,
		int spriteRightX,
		int spriteBottomY,
		GLint drawLeftX,
		GLint drawTopY,
		GLint drawRightX,
		GLint drawBottomY);
	//draw the specified sprite image with its top-left corner at the specified coordinate
	static void (SpriteSheet::* renderSpriteAtScreenPosition)(
		int spriteHorizontalIndex, int spriteVerticalIndex, GLint drawLeftX, GLint drawTopY);
	//draw the specified sprite image to the screen with its top-left corner at the specified coordinate
	void renderSpriteAtScreenPositionOpenGL(
		int spriteHorizontalIndex, int spriteVerticalIndex, GLint drawLeftX, GLint drawTopY);
	//draw the specified sprite image to the renderer with its top-left corner at the specified coordinate
	void renderSpriteAtScreenPositionRenderer(
		int spriteHorizontalIndex, int spriteVerticalIndex, GLint drawLeftX, GLint drawTopY);
	//draw the specified sprite image with its center at the specified coordinate
	void renderSpriteCenteredAtScreenPosition(
		int spriteHorizontalIndex, int spriteVerticalIndex, float drawCenterX, float drawCenterY);
	//set the color for a rectangle
	static void (* setRectangleColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	//set the color for a rectangle to use with OpenGL
	static void setRectangleColorOpenGL(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	//set the color for a rectangle to use with the renderer
	static void setRectangleColorRenderer(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	//render a rectangle filled with the specified color at the specified region of the screen
	static void renderFilledRectangle(
		GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, GLint leftX, GLint topY, GLint rightX, GLint bottomY);
	//render a rectangle with the current color at the specified region
	static void (* renderPreColoredRectangle)(GLint leftX, GLint topY, GLint rightX, GLint bottomY);
	//render a rectangle with the current color at the specified region of the screen
	static void renderPreColoredRectangleOpenGL(GLint leftX, GLint topY, GLint rightX, GLint bottomY);
	//render a rectangle with the current color at the specified region in the renderer
	static void renderPreColoredRectangleRenderer(GLint leftX, GLint topY, GLint rightX, GLint bottomY);
	//render a rectangle outline using the specified color at the specified region of the screen
	static void renderRectangleOutline(
		GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, GLint leftX, GLint topY, GLint rightX, GLint bottomY);
};
