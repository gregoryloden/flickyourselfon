// Designed 2014-10-11
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Insets;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.event.HierarchyBoundsListener;
import java.awt.event.HierarchyEvent;
import java.awt.geom.AffineTransform;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.RandomAccessFile;
import javax.imageio.ImageIO;
import javax.swing.JFrame;
import javax.swing.JPanel;
public class flickyourselfon extends JPanel implements MouseListener, MouseMotionListener, KeyListener, HierarchyBoundsListener {
	public static int screenwidth() {
		return Toolkit.getDefaultToolkit().getScreenSize().width;
	}
	public static int screenheight() {
		return Toolkit.getDefaultToolkit().getScreenSize().height;
	}
	//constants
	public static final int BASE_WIDTH = 199;
	public static final int BASE_HEIGHT = 149;
	public static final int SPRITE_WIDTH = 11;
	public static final int SPRITE_HEIGHT = 19;
	public static final int SPRITE_FRAMES = 9;
	public static final int TILE_SIZE = 6;
	public static final int MAP_HEIGHTS = 16; // height = blue / 16
	public static final int MAP_TILES = 64; // tile = green / 4
	public static final int MAP_HEIGHTS_FACTOR = 256 / MAP_HEIGHTS;
	public static final int MAP_TILES_FACTOR = 256 / MAP_TILES;
	public static final int FPS = 60;
	public static final int WALKING_FRAMES = 15;
	public static final int KICKING_FRAMES = 20;
	public static final int STARTING_PLAYER_Z = 0;
	public static final int BOOT_TILE = 37;
	public static final int DEFAULT_UP_KEY = KeyEvent.VK_UP;
	public static final int DEFAULT_DOWN_KEY = KeyEvent.VK_DOWN;
	public static final int DEFAULT_LEFT_KEY = KeyEvent.VK_LEFT;
	public static final int DEFAULT_RIGHT_KEY = KeyEvent.VK_RIGHT;
	public static final int DEFAULT_BOOT_KEY = KeyEvent.VK_SPACE;
	public static final int PAUSE_KEY = KeyEvent.VK_ESCAPE;
	public static final int PAUSE_MENU_UP_KEY = KeyEvent.VK_UP;
	public static final int PAUSE_MENU_DOWN_KEY = KeyEvent.VK_DOWN;
	public static final int PAUSE_MENU_LEFT_KEY = KeyEvent.VK_LEFT;
	public static final int PAUSE_MENU_RIGHT_KEY = KeyEvent.VK_RIGHT;
	public static final int PAUSE_MENU_SELECT_KEY = KeyEvent.VK_ENTER;
	public static final int PAUSE_MENU_SELECT_KEY2 = KeyEvent.VK_SPACE;
	public static final int PAUSE_MENU_BACK_KEY = KeyEvent.VK_BACK_SPACE;
	public static final int OPTIONS_BYTE_COUNT =
		//version number, 1 int
		4 +
		//key bindings, 5 ints
		5 * 4 +
		//pixel width, 2 doubles
		2 * 8;
	public static final int SAVE_FILE_SIZE = OPTIONS_BYTE_COUNT;
	public static final int SAVE_FILE_VERSION = 1;
	public static final int[] ANIMATION_NEXT_FRAME_INDICES =   new int[] {1, 2, 3, 0, 0, 6, 7, 8, 5, 10, 11, 5};
	public static final int[] ANIMATION_FRAME_SPRITE_INDICES = new int[] {0, 1, 0, 2, 3, 4, 5, 4, 6,  7,  8, 7};
	public static final int[] PAUSE_MENU_OPTION_COUNTS = new int[] {3, 5, 8};
	public static final double HALF_SQRT2 = Math.sqrt(2) * 0.5;
	public static final double SPEED = 0.625;
	public static final double DIAGONAL_SPEED = SPEED * HALF_SQRT2;
	public static final double STARTING_PLAYER_X = 179.5;
	public static final double STARTING_PLAYER_Y = 166.5;
	public static final double SMALL_DISTANCE = 1.0 / 65536.0;
	public static final double BOUNDING_BOX_RIGHT_OFFSET = SPRITE_WIDTH * 0.5;
	public static final double BOUNDING_BOX_LEFT_OFFSET = -BOUNDING_BOX_RIGHT_OFFSET;
	public static final double BOUNDING_BOX_TOP_OFFSET = 4.5;
	public static final double BOUNDING_BOX_BOTTOM_OFFSET = 9.5;
	public static final double BOOT_CENTER_PLAYER_Y_OFFSET = 8.0;
	public static final double DEFAULT_PIXEL_SIZE = 3.0;
	public static File playerImageFile = new File("images/player.png");
	public static File tilesImageFile = new File("images/tiles.png");
	public static File floorImageFile = new File("images/floor.png");
	public static File fontImageFile = new File("images/font.png");
	public static File radiotowerImageFile = new File("images/radiotower.png");
	public static RandomAccessFile saveFile = null;
	//function outputs
	public static int mouseMapX = 0;
	public static int mouseMapY = 0;
	public static int pauseMenuCurrentOption = 0;
	public static int pauseMenuDrawY = 0;
	//screen setup
	public JFrame window = new JFrame("Flick Yourself On");
	public double pixelWidth = DEFAULT_PIXEL_SIZE;
	public double pixelHeight = DEFAULT_PIXEL_SIZE;
	public int windowWidth = 1;
	public int windowHeight = 1;
	//display
	public BufferedImage playerImage = null;
	public BufferedImage tilesImage = null;
	public BufferedImage floorImage = null;
	public BufferedImage fontImage = null;
	public BufferedImage radiotowerImage = null;
	public BufferedImage[] tileList = new BufferedImage[MAP_TILES];
	public BufferedImage[][] tiles = null;
	public int[][] tileIndices = null;
	public int[][] heights = null;
	public BufferedImage[][] playerSprites = new BufferedImage[4][SPRITE_FRAMES];
	public BufferedImage[] currentPlayerSprites = null;
	public BufferedImage[] charImages = new BufferedImage[256];
	public int[] charWidths = new int[256];
	public int charLineHeight = 0;
	public BufferedImage keyBackground = null;
	//0=right 1=up 2=left 3=down
	public int facing = 3;
	public int idleAnimationIndex = 0;
	public int animationFrame = -1;
	public int animationIndex = 0;
	public int mapWidth = 0;
	public int mapHeight = 0;
	public boolean painting = false;
	//game state
	public double px = STARTING_PLAYER_X;
	public double py = STARTING_PLAYER_Y;
	public int pz = STARTING_PLAYER_Z;
	public int vertKey = 0;
	public int horizKey = 0;
	public boolean pressedVertLast = true;
	public int bootMapX = -1;
	public int bootMapY = -1;
	public int upKey = DEFAULT_UP_KEY;
	public int downKey = DEFAULT_DOWN_KEY;
	public int leftKey = DEFAULT_LEFT_KEY;
	public int rightKey = DEFAULT_RIGHT_KEY;
	public int bootKey = DEFAULT_BOOT_KEY;
	public String[] keyStrings = new String[] {
		"\u0080",
		"\u0081",
		"\u0082",
		"\u0083",
		KeyEvent.getKeyText(KeyEvent.VK_SPACE),
	};
	public boolean kicking = false;
	//1=grabbing boot 2=climbing 3=falling
	public int kickingAction = 0;
	public double[] kickingDX = null;
	public double[] kickingDY = null;
	//pause states
	//0-gameplay 1-pause
	public int pauseOption = 0;
	public int pauseMenuOption = -1;
	public boolean choosingKey = false;
	//debug options
	public boolean debugDrawBoundingBox = false;
	//editor constants
	public static final int EDITOR_MARGIN_RIGHT = 250;
	public static final int EDITOR_MARGIN_BOTTOM = 100;
	public static final int EDITOR_TILE_ROWS = MAP_TILES >> 3;
	public static final int EDITOR_HEIGHT_ROW_START = EDITOR_TILE_ROWS + 1;
	public static final int EDITOR_HEIGHT_ROWS = MAP_HEIGHTS >> 3;
	public static final int EDITOR_BRUSH_SIZE_ROW_START = EDITOR_HEIGHT_ROW_START + EDITOR_HEIGHT_ROWS + 1;
	public static final int EDITOR_NOISE_ROW_START = EDITOR_BRUSH_SIZE_ROW_START + 2;
	public static final int EDITOR_NOISE_ROWS = 2;
	public static final int EDITOR_NOISE_MAX_COUNT = EDITOR_NOISE_ROWS << 3;
	public static final int EDITOR_NOISE_BOX_X = 42;
	public static final int EDITOR_NOISE_BOX_Y = (EDITOR_NOISE_ROW_START - 1) * 28 + 19;
	public static final int EDITOR_NOISE_BOX_W = 53;
	public static final int EDITOR_NOISE_BOX_H = 15;
	public static final double EDITOR_SPEED_BOOST = 8.0;
	//editor settings
	public boolean editor = false;
	public static Color editorBlue = new Color(64, 192, 192);
	public static Color editorGray = new Color(160, 160, 160);
	public static Color editorYellow = new Color(224, 224, 64);
	public static Color editorShadow = new Color(0, 0, 0, 128);
	public static Color editorSaveRed = new Color(224, 96, 96);
	public static Color editorSaveGreen = new Color(96, 192, 96);
	public static Color editorSaveSuccessRed = new Color(192, 0, 0);
	public static Color editorSaveSuccessGreen = new Color(0, 192, 0);
	public static Color editorExportGreen = new Color(32, 128, 64);
	public static Color editorExportRed = new Color(160, 64, 64);
	public static Color editorBrushSizePurple = new Color(144, 96, 160);
	public static Color editorNoiseChanceColor = Color.WHITE;
	public boolean selectingTile = false;
	public boolean selectingHeight = false;
	public boolean noisyTileSelection = false;
	public int selectedTileIndex = 0;
	public int selectedHeight = 0;
	public int selectedBrushSize = 0;
	public int[] noiseTileIndices = new int[EDITOR_NOISE_MAX_COUNT];
	public int[] noiseTileChances = new int[EDITOR_NOISE_MAX_COUNT];
	public int noiseTileCount = 0;
	public Font selectedHeightFont = new Font("Dialog", Font.BOLD, 16);
	public Font saveFont = new Font("Monospaced", Font.BOLD, 44);
	public Font exportFont = new Font("Monospaced", Font.BOLD, 22);
	public Font noiseFont = new Font("Monospaced", Font.BOLD, 16);
	public Font noiseChanceFont = new Font("Monospaced", Font.BOLD, 12);
	public int saveSuccess = 1;
	public int exportSuccess = 0;
	public boolean editorSpeeding = false;
	public static void main(String[] args) {
		try {
			//initialize game
			flickyourselfon thepanel = new flickyourselfon();
			for (int i = 0; i < args.length; i++) {
				if (args[i].equals("-editor"))
					thepanel.editor = true;
				else if (args[i].equals("-drawboundingbox"))
					thepanel.debugDrawBoundingBox = true;
/*
else if (args[i].equals("-dothing")) {
	BufferedImage image = ImageIO.read(floorImageFile);
	int[] rgbs = image.getRGB(0, 0, image.getWidth(), image.getHeight(), null, 0, image.getWidth());
	for (int j = 0; j < rgbs.length; j++) {
		int pixel = rgbs[j];
		int tilenum = ((pixel >> 8) & 255) / MAP_TILES_FACTOR;
		if (pixel != 0) {
//VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
if (tilenum >= 19 && tilenum < 27)
	tilenum += 10;
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
			rgbs[j] = (pixel & 0xFFFF00FF) | (((tilenum + 1) * MAP_TILES_FACTOR - 1) << 8);
		}
	}
	image.setRGB(0, 0, image.getWidth(), image.getHeight(), rgbs, 0, image.getWidth());
	ImageIO.write(image, "png", floorImageFile);
}
/**/
			}
			//initialize display window
			thepanel.window.setContentPane(thepanel);
			thepanel.window.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
			thepanel.window.setVisible(true);
			thepanel.resizeWindow(1);
			Insets insets = thepanel.window.getInsets();
			thepanel.window.setMinimumSize(new Dimension(BASE_WIDTH + insets.left + insets.right, BASE_HEIGHT + insets.top + insets.bottom));
			thepanel.setFocusable(true);
			thepanel.requestFocus();
			thepanel.window.toFront();
			thepanel.addHierarchyBoundsListener(thepanel);
			//run the game!
			long now = System.currentTimeMillis();
			int next = 1;
			while (true) {
				while (System.currentTimeMillis() - now < (next * 1000 / FPS))
					Thread.sleep(1);
				thepanel.update();
				if (!thepanel.painting)
				{
					thepanel.painting = true;
					thepanel.repaint();
				}
				next = next % FPS + 1;
				if (next == 1)
					now = System.currentTimeMillis();
			}
		} catch(Exception e) {
			e.printStackTrace();
		}
	}
	public flickyourselfon() throws Exception {
		addKeyListener(this);
		addMouseListener(this);
		addMouseMotionListener(this);
setBackground(new Color(48, 0, 48));
		resetGame();
		saveFile = new RandomAccessFile(new File("fyo.sav"), "rw");
		if (saveFile.length() < SAVE_FILE_SIZE)
			save(true, true);
		else
			load(true, true);
	}
	public void resetGame() throws Exception {
		loadImages();
		resetGameState();
	}
	public void loadImages() throws Exception {
		//load all sprites
		playerImage = ImageIO.read(playerImageFile);
		tilesImage = ImageIO.read(tilesImageFile);
		floorImage = ImageIO.read(floorImageFile);
		fontImage = ImageIO.read(fontImageFile);
		radiotowerImage = ImageIO.read(radiotowerImageFile);
		//setup the level
		//setup the player sprites
		for (int y = 0; y < 4; y++) {
			BufferedImage[] playerSpritesY = playerSprites[y];
			for (int x = 0; x < SPRITE_FRAMES; x++)
				playerSpritesY[x] = playerImage.getSubimage(x * SPRITE_WIDTH, y * SPRITE_HEIGHT, SPRITE_WIDTH, SPRITE_HEIGHT);
		}
		//setup the tile list
		int tileCount = tilesImage.getWidth() / TILE_SIZE;
		for (int i = 0; i < tileCount; i++)
			tileList[i] = tilesImage.getSubimage(i * TILE_SIZE, 0, TILE_SIZE, TILE_SIZE);
		BufferedImage blackTile = new BufferedImage(6, 6, BufferedImage.TYPE_INT_RGB);
		for (int i = tileCount; i < MAP_TILES; i++)
			tileList[i] = blackTile;
		//setup the floor map
		mapWidth = floorImage.getWidth();
		mapHeight = floorImage.getHeight();
		tiles = new BufferedImage[mapHeight][mapWidth];
		tileIndices = new int[mapHeight][mapWidth];
		heights = new int[mapHeight][mapWidth];
		for (int y = 0; y < mapHeight; y++) {
			BufferedImage[] tilesY = tiles[y];
			int[] tileIndicesY = tileIndices[y];
			int[] heightsY = heights[y];
			for (int x = 0; x < mapWidth; x++) {
				int pixel = floorImage.getRGB(x, y);
				int railComponent;
				//consider anything at the max height to be filler
				if ((heightsY[x] = (pixel & 255) / MAP_HEIGHTS_FACTOR) == MAP_HEIGHTS - 1) {
					tilesY[x] = null;
					tileIndicesY[x] = -1;
				//if it has a red component, it's a rail/switch
				//0 is technically sine-wave/rail/group-0, but group 0 is disallowed
				} else if ((railComponent = ((pixel >> 16) & 255)) != 0) {
					//bits 0-1: which wave it's part of
					//bit 2: rail (0) or switch (1)
					//bits 3-7: which switch group it's part of (exclude 0, 31 possible groups)
				//it's just a regular map tile
				} else
					tilesY[x] = tileList[tileIndicesY[x] = ((pixel >> 8) & 255) / MAP_TILES_FACTOR];
			}
		}
		//go find the boot tile
		for (int y = mapHeight - 1; y >= 0; y--) {
			int[] tileIndicesY = tileIndices[y];
			for (int x = 0; x < mapWidth; x++) {
				if (tileIndicesY[x] == BOOT_TILE) {
					bootMapX = x;
					bootMapY = y;
					y = 0;
					break;
				}
			}
		}
		//build font images
		int fontimagewidth = fontImage.getWidth();
		int toprow = 0;
		int bottomrow = 0;
		int nextchar = ' ';
		while (true) {
			for (; true; bottomrow++) {
				boolean foundpixel = false;
				for (int checkx = 0; checkx < fontimagewidth; checkx++) {
					if (fontImage.getRGB(checkx, bottomrow) != 0) {
						foundpixel = true;
						break;
					}
				}
				if (!foundpixel)
					break;
			}
			//we hit the end, this row of characters is empty
			if (bottomrow == toprow)
				break;
			int leftcol = 0, rightcol = 0;
			while (true) {
				boolean foundpixel = false;
				for (int rowy = toprow; rowy < bottomrow; rowy++) {
					if (fontImage.getRGB(rightcol, rowy) != 0) {
						foundpixel = true;
						break;
					}
				}
				if (!foundpixel) {
					//we hit the end, this character is empty
					if (leftcol == rightcol)
						break;
					else {
						BufferedImage c = fontImage.getSubimage(leftcol, toprow, rightcol - leftcol, bottomrow - toprow);
						charImages[nextchar] = c;
						rightcol++;
						charWidths[nextchar] = rightcol - leftcol;
						nextchar++;
						leftcol = rightcol;
					}
				} else
					rightcol++;
			}
			bottomrow++;
			charLineHeight = Math.max(charLineHeight, bottomrow - toprow);
			toprow = bottomrow;
		}
		//certain characters need adjustments
		charImages[' '] = null;
		charImages['"'].setRGB(1, 0, 0);
		int fontimageheight = fontImage.getHeight();
		int kbx = fontimagewidth - 1;
		int kby = fontimageheight - 1;
		//start 8 pixels in
		int checkx = fontimagewidth - 9;
		int checky = fontimageheight - 9;
		while (fontImage.getRGB(checkx, kby) != 0)
			kby--;
		while (fontImage.getRGB(kbx, checky) != 0)
			kbx--;
		keyBackground = fontImage.getSubimage(kbx + 1, kby + 1, fontimagewidth - kbx - 1, fontimageheight - kby - 1);
	}
	public void resetGameState() {
		px = STARTING_PLAYER_X;
		py = STARTING_PLAYER_Y;
		pz = STARTING_PLAYER_Z;
		facing = 3;
		currentPlayerSprites = playerSprites[3];
		vertKey = 0;
		horizKey = 0;
		pressedVertLast = true;
		saveSuccess = 1;
		exportSuccess = 0;
		idleAnimationIndex = 0;
	}
	////////////////////////////////Draw////////////////////////////////
	public void paintComponent(Graphics g) {
super.paintComponent(g);
		//adjust the screenspace
		Graphics2D g2 = (Graphics2D)(g);
		AffineTransform oldTransform = g2.getTransform();
		g2.transform(AffineTransform.getScaleInstance(pixelWidth, pixelHeight));
		//start drawing
		drawFloor(g);
		if (!editor)
			g.drawImage(radiotowerImage, 423 - (int)(Math.floor(px)), -32 - (int)(Math.floor(py)), null);
		g.drawImage(currentPlayerSprites[ANIMATION_FRAME_SPRITE_INDICES[animationIndex]],
			(BASE_WIDTH - SPRITE_WIDTH) / 2, (BASE_HEIGHT - SPRITE_HEIGHT) / 2, null);
		//bounding box
		if (debugDrawBoundingBox) {
			g.setColor(new Color(255, 255, 255, 192));
			g.fillRect((int)(BASE_WIDTH / 2.0 + BOUNDING_BOX_LEFT_OFFSET), (int)(BASE_HEIGHT / 2.0 + BOUNDING_BOX_TOP_OFFSET),
				(int)(BOUNDING_BOX_RIGHT_OFFSET - BOUNDING_BOX_LEFT_OFFSET), (int)(BOUNDING_BOX_BOTTOM_OFFSET - BOUNDING_BOX_TOP_OFFSET));
		}
		//pause menu
		if (pauseOption >= 1 && pauseOption <= 3) {
			g.setColor(new Color(48, 0, 48, 160));
			g.fillRect(0, 0, BASE_WIDTH, BASE_HEIGHT);
			pauseMenuCurrentOption = 0;
			String title;
			int titleY;
			//different pause menus, pauseMenuDrawY is global for repeat render calls
			if (pauseOption == 1) {
				pauseMenuDrawY = 52;
				titleY = 16;
				title = "Pause";
				renderNormalMenuOption(g, "display");
				renderNormalMenuOption(g, "controls");
				renderNormalMenuOption(g, "exit game");
			} else if (pauseOption == 2) {
				pauseMenuDrawY = 44;
				titleY = 12;
				title = "Display";
				renderNormalMenuOption(g, String.format("pixel width: %.3f", pixelWidth));
				renderNormalMenuOption(g, String.format("pixel height: %.3f", pixelHeight));
				renderNormalMenuOption(g, "defaults");
				renderNormalMenuOption(g, "accept");
				renderNormalMenuOption(g, "cancel");
			} else {
				pauseMenuDrawY = 28;
				titleY = 4;
				title = "Controls";
				renderKeyControlMenuOption(g, "up:");
				renderKeyControlMenuOption(g, "down:");
				renderKeyControlMenuOption(g, "left:");
				renderKeyControlMenuOption(g, "right:");
				renderKeyControlMenuOption(g, "boot:");
				pauseMenuDrawY += 2;
				renderNormalMenuOption(g, "defaults");
				renderNormalMenuOption(g, "accept");
				renderNormalMenuOption(g, "cancel");
			}
			//draw title text big
			g2.transform(AffineTransform.getScaleInstance(2, 2));
			drawString(g, title, (BASE_WIDTH / 2 - textWidth(title)) / 2, titleY);
		}
		//editor UI
		if (editor) {
			//hover box
			if ((noisyTileSelection ? (noiseTileCount > 0) : selectingTile) || selectingHeight) {
				int dx = BASE_WIDTH / 2 - (int)(Math.floor(px)) + (mouseMapX - selectedBrushSize) * TILE_SIZE;
				int dy = BASE_HEIGHT / 2 - (int)(Math.floor(py)) + (mouseMapY - selectedBrushSize) * TILE_SIZE;
				int drawSize = ((selectedBrushSize << 1) + 1) * TILE_SIZE;
				g2.translate(0.5, 0.5);
				g.setColor(Color.BLACK);
				g.drawRect(dx - 1, dy - 1, drawSize + 1, drawSize + 1);
				g.setColor(Color.WHITE);
				g.drawRect(dx - 2, dy - 2, drawSize + 3, drawSize + 3);
			}
			//blue backgrounds
			g2.setTransform(oldTransform);
			g.setColor(editorBlue);
			g.fillRect(windowWidth, 0, EDITOR_MARGIN_RIGHT, windowHeight);
			g.fillRect(0, windowHeight, windowWidth + EDITOR_MARGIN_RIGHT, EDITOR_MARGIN_BOTTOM);
			//tile images
			int leftX = windowWidth + 15;
			for (int y = 0; y < EDITOR_TILE_ROWS; y++) {
				int dy = 15 + y * 28;
				for (int x = 0; x < 8; x++) {
					int index = x + y * 8;
					int dx = leftX + x * 28;
					g.drawImage(tileList[index], dx, dy, 24, 24, null);
					g.setColor((index == selectedTileIndex && selectingTile && !noisyTileSelection) ? editorYellow : editorGray);
					g.drawRect(dx - 1, dy - 1, 25, 25);
					g.drawRect(dx - 2, dy - 2, 27, 27);
				}
			}
			//heights
			g.setFont(selectedHeightFont);
			int topRow = EDITOR_TILE_ROWS + 1;
			for (int x = 0; x < 8; x++) {
				int dx = leftX + x * 28;
				for (int y = 0; y < EDITOR_HEIGHT_ROWS; y++) {
					int heightNum = (x << 1) + y;
					int dy = 15 + (topRow + y) * 28;
					g.setColor(Color.BLACK);
					g.fillRect(dx, dy, 24, 24);
					g.setColor(editorYellow);
					g.drawString(String.valueOf(heightNum), dx + (heightNum < 10 ? 8 : 3), dy + 18);
					g.setColor((heightNum == selectedHeight && selectingHeight) ? editorYellow : editorGray);
					g.drawRect(dx - 1, dy - 1, 25, 25);
					g.drawRect(dx - 2, dy - 2, 27, 27);
				}
			}
			//brush sizes
			int topY2 = 15 + (topRow + 3) * 28;
			for (int x = 0; x < 8; x++) {
				int dx = leftX + x * 28;
				g.setColor(Color.BLACK);
				g.fillRect(dx, topY2, 24, 24);
				g.setColor(editorBrushSizePurple);
				g.fillRect(dx + 3, topY2 + 3, (x << 1) + 1, (x << 1) + 1);
				g.setColor((x == selectedBrushSize) ? editorYellow : editorGray);
				g.drawRect(dx - 1, topY2 - 1, 25, 25);
				g.drawRect(dx - 2, topY2 - 2, 27, 27);
			}
			//noise text
			g.setFont(noiseFont);
			g.setColor(editorYellow);
			g.drawString("Noise", windowWidth + EDITOR_NOISE_BOX_X + 2, EDITOR_NOISE_BOX_Y + 12);
			g.setColor(noisyTileSelection ? editorYellow : editorGray);
			g.drawRect(windowWidth + EDITOR_NOISE_BOX_X, EDITOR_NOISE_BOX_Y, EDITOR_NOISE_BOX_W, EDITOR_NOISE_BOX_H);
			//noise boxes
			g.setFont(noiseChanceFont);
			for (int y = 0; y < EDITOR_NOISE_ROWS; y++) {
				int dy = topY2 + (y + 2) * 28;
				for (int x = 0; x < 8; x++) {
					int dx = leftX + x * 28;
					int index = (y << 3) + x;
					if (index < noiseTileCount) {
						g.drawImage(tileList[noiseTileIndices[index]], dx, dy, 24, 24, null);
						g.setColor(editorNoiseChanceColor);
						g.drawString(String.valueOf(noiseTileChances[index]), dx + 3, dy + 12);
					} else {
						g.setColor(Color.BLACK);
						g.fillRect(dx, dy, 24, 24);
					}
					g.setColor(editorGray);
					g.drawRect(dx - 1, dy - 1, 25, 25);
					g.drawRect(dx - 2, dy - 2, 27, 27);
				}
			}
			//backgrounds for tile images, heights, brush sizes, and noise tiles
			g.setColor(editorGray);
			leftX -= 3;
			int topY = (EDITOR_TILE_ROWS + 1) * 28 + 12;
			topY2 -= 3;
			int topY3 = topY2 + 56;
			for (int i = 0; i < 4; i++) {
				g.drawRect(leftX - i, 12 - i, 225 + (i << 1), EDITOR_TILE_ROWS * 28 + 1 + (i << 1));
				g.drawRect(leftX - i, topY - i, 225 + (i << 1), EDITOR_HEIGHT_ROWS * 28 + 1 + (i << 1));
				g.drawRect(leftX - i, topY2 - i, 225 + (i << 1), 29 + (i << 1));
				g.drawRect(leftX - i, topY3 - i, 225 + (i << 1), EDITOR_NOISE_ROWS * 28 + 1 + (i << 1));
			}
			//save button
			g.setColor(editorSaveRed);
			g.drawRect(20, windowHeight + 20, 149, 59);
			g.drawRect(21, windowHeight + 21, 147, 57);
			g.fillRect(24, windowHeight + 24, 142, 52);
			g.setColor(editorSaveGreen);
			g.setFont(saveFont);
			g.drawString("Save", 43, windowHeight + 63);
			g.setColor(Color.WHITE);
			g.fillRect(181, windowHeight + 31, 38, 38);
			g.setColor(editorGray);
			g.drawRect(180, windowHeight + 30, 39, 39);
			if (saveSuccess == 1) {
				g.setColor(editorSaveSuccessGreen);
				g.drawString("\u2713", 182, windowHeight + 65);
			} else if (saveSuccess == -1) {
				g.setColor(editorSaveSuccessRed);
				g.drawString("\u2715", 179, windowHeight + 65);
			}
			//export map button
			g.setColor(editorExportGreen);
			g.drawRect(240, windowHeight + 20, 149, 59);
			g.drawRect(241, windowHeight + 21, 147, 57);
			g.fillRect(244, windowHeight + 24, 142, 52);
			g.setColor(editorExportRed);
			g.setFont(exportFont);
			g.drawString("Export Map", 250, windowHeight + 56);
			g.setColor(Color.WHITE);
			g.fillRect(401, windowHeight + 31, 38, 38);
			g.setColor(editorGray);
			g.drawRect(400, windowHeight + 30, 39, 39);
			if (exportSuccess == 1) {
				g.setFont(saveFont);
				g.setColor(editorSaveSuccessGreen);
				g.drawString("\u2713", 402, windowHeight + 65);
			} else if (exportSuccess == -1) {
				g.setFont(saveFont);
				g.setColor(editorSaveSuccessRed);
				g.drawString("\u2715", 399, windowHeight + 65);
			}
		}
		painting = false;
	}
	public void drawFloor(Graphics g) {
		int addX = BASE_WIDTH / 2 - (int)(Math.floor(px));
		int addY = BASE_HEIGHT / 2 - (int)(Math.floor(py));
		int tileMinY = Math.max(-addY / TILE_SIZE, 0);
		int tileMaxY = Math.min((BASE_HEIGHT - addY - 1) / TILE_SIZE + 1, mapHeight);
		int tileMinX = Math.max(-addX / TILE_SIZE, 0);
		int tileMaxX = Math.min((BASE_WIDTH - addX - 1) / TILE_SIZE + 1, mapWidth);
		for (int y = tileMinY; y < tileMaxY; y++) {
			BufferedImage[] tilesY = tiles[y];
			for (int x = tileMinX; x < tileMaxX; x++)
				g.drawImage(tilesY[x], x * TILE_SIZE + addX, y * TILE_SIZE + addY, null);
		}
		if (selectingHeight) {
			g.setColor(editorShadow);
			for (int y = tileMinY; y < tileMaxY; y++) {
				int[] heightsY = heights[y];
				for (int x = tileMinX; x < tileMaxX; x++) {
					if (heightsY[x] != selectedHeight)
						g.fillRect(x * TILE_SIZE + addX, y * TILE_SIZE + addY, 6, 6);
				}
			}
		}
	}
	public void renderNormalMenuOption(Graphics g, String s) {
		if (pauseMenuCurrentOption == pauseMenuOption)
			s = "< " + s + " >";
		drawString(g, s, (BASE_WIDTH - textWidth(s)) / 2, pauseMenuDrawY);
		pauseMenuDrawY += charLineHeight;
		pauseMenuCurrentOption++;
	}
	public void drawString(Graphics g, String s, int leftx, int drawy) {
		int slength = s.length();
		int drawx = leftx;
		for (int i = 0; i < slength; i++) {
			char c = s.charAt(i);
			if (c == '\n') {
				drawx = leftx;
				drawy += charLineHeight;
			} else {
				g.drawImage(charImages[c], drawx, drawy, null);
				drawx += charWidths[c];
			}
		}
	}
	public void renderKeyControlMenuOption(Graphics g, String s) {
		int kbh = keyBackground.getHeight();
		int width;
		if (choosingKey && pauseMenuCurrentOption == pauseMenuOption) {
			s += " [press any key]";
			width = textWidth(s);
		} else {
			int originalwidth = textWidth(s);
			String letter = keyStrings[pauseMenuCurrentOption];
			int lw = textWidth(letter);
			int drawx;
			//accounts for the key background offsets and the two spaces for a single letter
			s += "      ";
			int ll = letter.length();
			if (ll > 1) {
				//readjust the string and the x values
				int spacewidth = charWidths[' '];
				//pixel space = letter count + 3
				int spacecount = (lw + (ll + 3) * 2 - spacewidth - 1) / spacewidth;
				for (int j = spacecount; j > 2; j--)
					s += " ";
				width = textWidth(s);
				drawx = (BASE_WIDTH - width) / 2 + originalwidth + 2;
				//draw the left half
				int kbw = keyBackground.getWidth();
				int halfx = kbw / 2;
				int halfx2 = kbw - halfx;
				g.drawImage(keyBackground.getSubimage(0, 0, halfx, kbh), drawx, pauseMenuDrawY, null);
				//draw the middle slices
				BufferedImage slice = keyBackground.getSubimage(halfx, 0, 1, kbh);
				int textwidth = (spacecount * spacewidth);
				int maxx = drawx + (textwidth + 9) - halfx2;
				for (int newdrawx = drawx + halfx; newdrawx < maxx; newdrawx++)
					g.drawImage(slice, newdrawx, pauseMenuDrawY, null);
				//draw the end half
				g.drawImage(keyBackground.getSubimage(halfx, 0, halfx2, kbh), maxx, pauseMenuDrawY, null);
				//fix lw
				lw += 6 - textwidth;
			} else {
				width = textWidth(s);
				drawx = (BASE_WIDTH - width) / 2 + originalwidth + 2;
				g.drawImage(keyBackground, drawx, pauseMenuDrawY, null);
			}
			drawString(g, letter, drawx + (15 - lw) / 2, pauseMenuDrawY + 3);
		}
		if (pauseMenuCurrentOption == pauseMenuOption) {
			s = "< " + s + " >";
			width = textWidth(s);
		}
		drawString(g, s, (BASE_WIDTH - width) / 2, pauseMenuDrawY + 3);
		pauseMenuDrawY += kbh + 1;
		pauseMenuCurrentOption++;
	}
	////////////////////////////////Update////////////////////////////////
	public void update() {
		if (pauseOption != 0)
			return;
		int newFacing = -1;
		if (kicking) {
			//no current kicking action, let's try to find one
			if (kickingAction == 0) {
				//player has the boot
				if (idleAnimationIndex == 5) {
					if (animationFrame == 0 && animationIndex == 10) {
						int lowmapx = (int)(px + BOUNDING_BOX_LEFT_OFFSET) / TILE_SIZE;
						int highmapx = (int)(px + BOUNDING_BOX_RIGHT_OFFSET) / TILE_SIZE;
						//kicking north, climbing, jumping off, or kicking a switch
						if (facing == 1) {
							int checky = (int)(py + BOUNDING_BOX_BOTTOM_OFFSET) / TILE_SIZE;
							boolean canclimb = true;
							boolean canfall = false;
							int fallheight = -1;
							for (int offsety = 1; offsety <= 2; offsety++) {
								int[] heightsY = heights[checky - offsety];
								for (int itermapx = lowmapx; itermapx <= highmapx; itermapx++) {
									int height;
									if ((height = heightsY[itermapx]) != pz + offsety)
										canclimb = false;
									if (offsety == 1 && height < pz) {
										if (fallheight == -1) {
											fallheight = height;
											canfall = true;
										} else if (height != fallheight)
											canfall = false;
									}
								}
							}
							if (canclimb) {
								kickingDX = new double[] {0.0};
								double kf2 = KICKING_FRAMES * 2.0;
								double mindist = TILE_SIZE * -2.0 / (kf2 * kf2 * kf2 * kf2);
								//x^3:	1	-6	6
								//x^4:	-1	14	-36	24
								//(x^3+x^4)/2:	39/2	-226/2	204/2	24/2
								kickingDY = new double[] {19.5 * mindist, -113.0 * mindist, 102.0 * mindist, 12.0 * mindist};
								pz += 2;
								kickingAction = 2;
							} else if (canfall) {
								kickingDX = new double[] {0.0};
								double kf2 = KICKING_FRAMES * 2.0;
								double mindist = -TILE_SIZE / (kf2 * kf2 * kf2);
								//vy=c(x + d)(x - i)
								//...
								//y=cx(2x^2+(3d-3i)x-6id)
								//(i, j) is chosen as the middle coordinate
								//j=ci(2i^2+(3d-3i)i-6id)
								//1=c(2+3d-3i-6id)
								//...
								//d=(2j-3ij+i^3)/(6ij-3i^2-3j)
								//c=(i^2-2ij+j)/((2i^2)(i-1)^2)
								//(0,0)+(2/3,1.5)+(1,1) curve:	51791/8,	-1146/8,	-54/8
								//(0,0)+(2/3,2)+(1,1) curve:	50684/8,	1176/8,	-216/8
								kickingDY = new double[] {6335.5 * mindist, 147.0 * mindist, -27.0 * mindist};
								pz = fallheight;
								kickingAction = 3;
							}
						//kicking down, jumping off
						} else if (facing == 3) {
							int checky = (int)(py + BOUNDING_BOX_TOP_OFFSET) / TILE_SIZE;
							boolean canfall = false;
							boolean stillfalling = true;
							int offset = 1;
							int fallheight;
							for (; true; offset++) {
								fallheight = -1;
								int[] heightsY = heights[checky + offset];
								for (int itermapx = lowmapx; itermapx <= highmapx; itermapx++) {
									int height;
									//can't fall if it's not below
									if ((height = heightsY[itermapx]) >= pz) {
										stillfalling = false;
										break;
									//set the fall height
									} else if (fallheight == -1)
										fallheight = height;
									//we already had a fall height but this one is different
									else if (fallheight != height) {
										stillfalling = false;
										break;
									}
								}
								if (!stillfalling)
									break;
								//we found a height on ground-plane tiles
								else if ((fallheight & 1) == 0) {
									canfall = true;
									break;
								}
							}
							if (canfall) {
								kickingDX = new double[] {0.0};
								double kf2 = KICKING_FRAMES * 2.0;
								double mindist = (-TILE_SIZE * offset) / (kf2 * kf2);
								//x-2x^2:	42,	-4
								kickingDY = new double[] {42.0 * mindist, -4.0 * mindist};
								pz = fallheight;
								kickingAction = 3;
							}
						//kicking to the side, jumping off or kicking a switch
						} else {
							int checky = (int)(py + BOUNDING_BOX_BOTTOM_OFFSET) / TILE_SIZE;
							int diff = highmapx - lowmapx + 1;
							if (facing == 0) {
								lowmapx += diff;
								highmapx += diff;
							} else {
								lowmapx -= diff;
								highmapx -= diff;
							}
							int useoffset = 0;
							int fallheight = -1;
							for (int offset = 0; true; offset++) {
								boolean canfall = false;
								int newfallheight = -1;
								int[] heightsY = heights[checky + offset];
								for (int itermapx = lowmapx; itermapx <= highmapx; itermapx++) {
									int height;
									if ((height = heightsY[itermapx]) <= pz - offset * 2 && height != pz) {
										//has to be a ground-plane tile
										if ((height & 1) == 0) {
											if (newfallheight == -1) {
												newfallheight = height;
												canfall = true;
											} else if (newfallheight != height)
												canfall = false;
										//slope tile, this row is not valid but lower rows might be
										} else {
											newfallheight = 0;
											canfall = false;
											break;
										}
									//it's too high- stop looking for fall heights
									} else {
										canfall = false;
										newfallheight = -1;
										break;
									}
								}
								//we found tiles we can fall to- set the fall height
								if (canfall) {
									fallheight = newfallheight;
									useoffset = offset;
								//we reached a set of tiles that's too high, stop looking for fall heights
								} else if (newfallheight == -1)
									break;
							}
							//there is at least one valid row of tiles to fall to
							if (fallheight != -1) {
								kickingDX = new double[] {(double)(facing == 0 ? TILE_SIZE : -TILE_SIZE) / KICKING_FRAMES};
								double kf2 = KICKING_FRAMES * 2.0;
								double topy = py + BOUNDING_BOX_TOP_OFFSET;
								double scalesize = ((int)(topy) / TILE_SIZE < checky) ?
									useoffset + checky - (topy - SMALL_DISTANCE) / TILE_SIZE :
									useoffset;
								double mindist = (-TILE_SIZE * scalesize) / (kf2 * kf2);
								//x-2x^2:	42,	-4
								kickingDY = new double[] {42.0 * mindist, -4.0 * mindist};
								pz = fallheight;
								kickingAction = 3;
							}
						}
					}
				//player kicked without a boot, possibly putting on the boot
				} else if (animationFrame == 0) {
					int lowmapx,
						highmapx,
						lowmapy,
						highmapy;
					boolean foundboot = false;
					if ((facing & 1) == 0) {
						lowmapy = (int)(py + BOUNDING_BOX_TOP_OFFSET) / TILE_SIZE;
						highmapy = (int)(py + BOUNDING_BOX_BOTTOM_OFFSET) / TILE_SIZE;
						lowmapx = (highmapx = (facing == 2 ?
							((int)(px + BOUNDING_BOX_LEFT_OFFSET) / TILE_SIZE - 1) :
							((int)(px + BOUNDING_BOX_RIGHT_OFFSET) / TILE_SIZE + 1)));
					} else {
						lowmapx = (int)(px + BOUNDING_BOX_LEFT_OFFSET) / TILE_SIZE;
						highmapx = (int)(px + BOUNDING_BOX_RIGHT_OFFSET) / TILE_SIZE;
						lowmapy = (highmapy = (facing == 1 ?
							((int)(py + BOUNDING_BOX_TOP_OFFSET) / TILE_SIZE - 1) :
							((int)(py + BOUNDING_BOX_BOTTOM_OFFSET) / TILE_SIZE + 1)));
					}
					for (int itermapy = lowmapy; itermapy <= highmapy; itermapy++) {
						for (int itermapx = lowmapx; itermapx <= highmapx; itermapx++) {
							if (tileIndices[itermapy][itermapx] == BOOT_TILE) {
								kickingAction = 1;
								kickingDX = new double[] {((itermapx * TILE_SIZE + 3.5) - px) / KICKING_FRAMES};
								kickingDY = new double[] {((itermapy * TILE_SIZE + 3.5) - (py + BOOT_CENTER_PLAYER_Y_OFFSET)) / KICKING_FRAMES};
								itermapy = highmapy;
								break;
							}
						}
					}
				}
			}
			//player is in the process of putting on the boot, climbing, or falling
			//this may become true in the above if branch even if it wasn't before it
			if (kickingAction != 0) {
				for (int i = kickingDX.length - 1; i >= 1; i--)
					kickingDX[i - 1] += kickingDX[i];
				for (int i = kickingDY.length - 1; i >= 1; i--)
					kickingDY[i - 1] += kickingDY[i];
				px += kickingDX[0];
				py += kickingDY[0];
			}
		} else {
			//move sqrt2/2 in each direction if both directions are pressed
			boolean bothPressed = ((vertKey & horizKey) != 0);
			double dist = bothPressed ? DIAGONAL_SPEED : SPEED;
			if (editor && editorSpeeding)
				dist *= EDITOR_SPEED_BOOST;
			if (horizKey == 1) {
				if (!bothPressed || !pressedVertLast)
					newFacing = 0;
				px += dist;
			} else if (horizKey == -1) {
				if (!bothPressed || !pressedVertLast)
					newFacing = 2;
				px -= dist;
			}
			if (vertKey == -1) {
				if (!bothPressed || pressedVertLast)
					newFacing = 1;
				py -= dist;
			} else if (vertKey == 1) {
				if (!bothPressed || pressedVertLast)
					newFacing = 3;
				py += dist;
			}
			// collide with walls and off-map area
			if (!editor) {
				int lowmapx = (int)(px + BOUNDING_BOX_LEFT_OFFSET) / TILE_SIZE;
				int highmapx = (int)(px + BOUNDING_BOX_RIGHT_OFFSET) / TILE_SIZE;
				int lowmapy = (int)(py + BOUNDING_BOX_TOP_OFFSET) / TILE_SIZE;
				int highmapy = (int)(py + BOUNDING_BOX_BOTTOM_OFFSET) / TILE_SIZE;
				int chosenx,
					choseny,
					iterlowx,
					iterhighx,
					iterlowy,
					iterhighy;
				boolean xside = false;
				boolean yside = false;
				if (horizKey == -1) {
					chosenx = lowmapx;
					iterlowx = lowmapx + 1;
					iterhighx = highmapx;
				} else {
					chosenx = highmapx;
					iterlowx = lowmapx;
					iterhighx = highmapx - 1;
				}
				if (vertKey == -1) {
					choseny = lowmapy;
					iterlowy = lowmapy + 1;
					iterhighy = highmapy;
				} else {
					choseny = highmapy;
					iterlowy = lowmapy;
					iterhighy = highmapy - 1;
				}
				//go through and check to see if any of the (possibly) new tiles are walls, except the corner
				for (int itermapy = iterlowy; itermapy <= iterhighy; itermapy++) {
					if (heights[itermapy][chosenx] != pz) {
						xside = true;
						break;
					}
				}
				for (int itermapx = iterlowx; itermapx <= iterhighx; itermapx++) {
					if (heights[choseny][itermapx] != pz) {
						yside = true;
						break;
					}
				}
				//if we haven't hit any of the side walls, we need to check the corner
				if (!xside && !yside && heights[choseny][chosenx] != pz) {
					if (horizKey == 0)
						yside = true;
					else if (vertKey == 0)
						xside = true;
					//compare the x-diff to the y-diff: the shorter one gets the wall designation
					else if ((
						(horizKey == -1) ?
							((lowmapx + 1) * TILE_SIZE - px - BOUNDING_BOX_LEFT_OFFSET) :
							(px + BOUNDING_BOX_RIGHT_OFFSET - highmapx * TILE_SIZE)
						) < (
						(vertKey == -1) ?
							((lowmapy + 1) * TILE_SIZE - py - BOUNDING_BOX_TOP_OFFSET) :
							(py + BOUNDING_BOX_BOTTOM_OFFSET - highmapy * TILE_SIZE)
					))
						xside = true;
					else
						yside = true;
				}
				if (xside) {
					if (horizKey == -1)
						px = (lowmapx + 1) * TILE_SIZE + SMALL_DISTANCE - BOUNDING_BOX_LEFT_OFFSET;
					else
						px = highmapx * TILE_SIZE - SMALL_DISTANCE - BOUNDING_BOX_RIGHT_OFFSET;
				}
				if (yside) {
					if (vertKey == -1)
						py = (lowmapy + 1) * TILE_SIZE + SMALL_DISTANCE - BOUNDING_BOX_TOP_OFFSET;
					else
						py = highmapy * TILE_SIZE - SMALL_DISTANCE - BOUNDING_BOX_BOTTOM_OFFSET;
				}
			}
		}
		// update the facing direction
		if (newFacing != -1) {
			currentPlayerSprites = playerSprites[newFacing];
			facing = newFacing;
		}
		//update the animation frame
		if ((vertKey | horizKey) != 0 || kicking) {
			animationFrame += 1;
			if (animationFrame >= (kicking ? KICKING_FRAMES : WALKING_FRAMES)) {
				animationFrame = 0;
				animationIndex = ANIMATION_NEXT_FRAME_INDICES[animationIndex];
				if (kicking && animationIndex == idleAnimationIndex) {
					kicking = false;
					//boot has been put on
					if (kickingAction == 1) {
						int tilex = (int)(px) / TILE_SIZE;
						int tiley = (int)(py + BOOT_CENTER_PLAYER_Y_OFFSET) / TILE_SIZE;
						tiles[tiley][tilex] = tileList[tileIndices[tiley][tilex] = 0];
						heights[tiley][tilex] = STARTING_PLAYER_Z;
						animationIndex = (idleAnimationIndex = 5);
					}
					kickingAction = 0;
				}
			}
		} else {
			animationFrame = -1;
			animationIndex = idleAnimationIndex;
		}
	}
	public int textWidth(String s) {
		int slength = s.length();
		int currentWidth = 0, maxWidth = 0;
		for (int i = 0; i < slength; i++) {
			char c = s.charAt(i);
			if (c == '\n')
				currentWidth = 0;
			else {
				currentWidth += charWidths[c];
				maxWidth = Math.max(maxWidth, currentWidth);
			}
		}
		return maxWidth;
	}
	////////////////////////////////Input////////////////////////////////
	public void keyPressed(KeyEvent evt) {
		int code = evt.getKeyCode();
		//main game
		if (pauseOption == 0) {
			//pause the game
			if (code == PAUSE_KEY) {
				//but not if we're in the editor
				if (!editor) {
					pauseOption = 1;
					pauseMenuOption = 0;
					//reset the movement to avoid movement-after-unpausing issues
					vertKey = 0;
					horizKey = 0;
				}
			} else if (code == upKey) {
				vertKey = -1;
				pressedVertLast = true;
			} else if (code == downKey) {
				vertKey = 1;
				pressedVertLast = true;
			} else if (code == leftKey) {
				horizKey = -1;
				pressedVertLast = false;
			} else if (code == rightKey) {
				horizKey = 1;
				pressedVertLast = false;
			//editor-only keypresses
			} else if (editor) {
				if (code == bootKey)
					editorSpeeding = true;
				else if (code == KeyEvent.VK_R) {
					try {
						resetGame();
					} catch(Exception e) {
						e.printStackTrace();
						System.exit(0);
					}
				}
			//non-editor-only keypresses
			} else if (code == bootKey) {
				if (!kicking) {
					animationFrame = -1;
					animationIndex = idleAnimationIndex + 4;
					kicking = true;
				}
			}
		//game is paused
		} else {
			//unpause the game
			if (code == PAUSE_KEY) {
				//if we were in a menu, revert changes
				if (pauseOption >= 2) {
					evt.setKeyCode(PAUSE_MENU_BACK_KEY);
					keyPressed(evt);
				}
				pauseOption = 0;
			//choosing a key
			} else if (choosingKey) {
				if (pauseMenuOption == 0) {
					upKey = code;
					keyStrings[0] = keyToString(evt);
				} else if (pauseMenuOption == 1) {
					downKey = code;
					keyStrings[1] = keyToString(evt);
				} else if (pauseMenuOption == 2) {
					leftKey = code;
					keyStrings[2] = keyToString(evt);
				} else if (pauseMenuOption == 3) {
					rightKey = code;
					keyStrings[3] = keyToString(evt);
				} else if (pauseMenuOption == 4) {
					bootKey = code;
					keyStrings[4] = keyToString(evt);
				}
				//if there was another key assigned to this, set that one to be chosen
				if (upKey == code && pauseMenuOption != 0)
					pauseMenuOption = 0;
				else if (downKey == code && pauseMenuOption != 1)
					pauseMenuOption = 1;
				else if (leftKey == code && pauseMenuOption != 2)
					pauseMenuOption = 2;
				else if (rightKey == code && pauseMenuOption != 3)
					pauseMenuOption = 3;
				else if (bootKey == code && pauseMenuOption != 4)
					pauseMenuOption = 4;
				//all keys are different, no longer choosing key
				else
					choosingKey = false;
			} else if (code == PAUSE_MENU_UP_KEY) {
				int menuOptionCount = PAUSE_MENU_OPTION_COUNTS[pauseOption - 1];
				pauseMenuOption = (pauseMenuOption + menuOptionCount - 1) % menuOptionCount;
			} else if (code == PAUSE_MENU_DOWN_KEY) {
				pauseMenuOption = (pauseMenuOption + 1) % PAUSE_MENU_OPTION_COUNTS[pauseOption - 1];
			} else if (code == PAUSE_MENU_LEFT_KEY) {
				if (pauseOption == 2) {
					if (pauseMenuOption == 0) {
						pixelWidth = (Math.round(pixelWidth * 8.0) - 1.0) * 0.125;
						resizeWindow(2);
					} else if (pauseMenuOption == 1) {
						pixelHeight = (Math.round(pixelHeight * 8.0) - 1.0) * 0.125;
						resizeWindow(2);
					}
				}
			} else if (code == PAUSE_MENU_RIGHT_KEY) {
				if (pauseOption == 2) {
					if (pauseMenuOption == 0) {
						pixelWidth = (Math.round(pixelWidth * 8.0) + 1.0) * 0.125;
						resizeWindow(2);
					} else if (pauseMenuOption == 1) {
						pixelHeight = (Math.round(pixelHeight * 8.0) + 1.0) * 0.125;
						resizeWindow(2);
					}
				}
			} else if (code == PAUSE_MENU_SELECT_KEY || code == PAUSE_MENU_SELECT_KEY2) {
				int lastOption;
				//enter sub-menu
				if (pauseOption == 1 && pauseMenuOption <= 1) {
					pauseOption = pauseMenuOption + 2;
					pauseMenuOption = 0;
				//exit game or revert options if in a sub-menu
				} else if (pauseMenuOption == (lastOption = PAUSE_MENU_OPTION_COUNTS[pauseOption - 1] - 1)) {
					if (pauseOption == 1)
						System.exit(0);
					else if (pauseOption >= 2) {
						//redo the event as a back
						evt.setKeyCode(PAUSE_MENU_BACK_KEY);
						keyPressed(evt);
					}
				//accept, save options
				//if this is true then we know we're in a sub-menu because the main menu would already have caught the event
				} else if (pauseMenuOption == lastOption - 1) {
					save(true, false);
					pauseMenuOption = pauseOption - 2;
					pauseOption = 1;
				//restore defaults
				//same as above, we're in a sub-menu
				} else if (pauseMenuOption == lastOption - 2) {
					//load pixel size defaults
					if (pauseOption == 2) {
						pixelWidth = DEFAULT_PIXEL_SIZE;
						pixelHeight = DEFAULT_PIXEL_SIZE;
						resizeWindow(2);
					//load key binding defaults
					} else if (pauseOption == 3) {
						keyStrings[0] = keyToString(upKey = DEFAULT_UP_KEY);
						keyStrings[1] = keyToString(downKey = DEFAULT_DOWN_KEY);
						keyStrings[2] = keyToString(leftKey = DEFAULT_LEFT_KEY);
						keyStrings[3] = keyToString(rightKey = DEFAULT_RIGHT_KEY);
						keyStrings[4] = keyToString(bootKey = DEFAULT_BOOT_KEY);
					}
				//set a key
				//if we get here, the pauseMenuOption is definitely on a key option
				} else if (pauseOption == 3)
					choosingKey = true;
			} else if (code == PAUSE_MENU_BACK_KEY) {
				load(true, false);
				//window size was reverted, restore the old window size
				if (pauseOption == 2)
					resizeWindow(2);
				//go back to the main pause menu at the position of this submenu
				pauseMenuOption = pauseOption - 2;
				pauseOption = 1;
			}
		}
	}
	public void keyReleased(KeyEvent evt) {
		int code = evt.getKeyCode();
		//main game
		if (pauseOption == 0) {
			if (code == upKey) {
				if (vertKey == -1)
					vertKey = 0;
			} else if (code == downKey) {
				if (vertKey == 1)
					vertKey = 0;
			} else if (code == leftKey) {
				if (horizKey == -1)
					horizKey = 0;
			} else if (code == rightKey) {
				if (horizKey == 1)
					horizKey = 0;
			//editor-only key releases
			} else if (editor) {
				if (code == bootKey)
					editorSpeeding = false;
			}
		}
	}
	public void mousePressed(MouseEvent evt) {
		int mouseX = evt.getX();
		int mouseY = evt.getY();
		if (editor) {
			//paint a tile
			if (mouseX < windowWidth && mouseY < windowHeight) {
				boolean isDoingNoisyTileSelection = noisyTileSelection && noiseTileCount > 0;
				if ((noisyTileSelection ? isDoingNoisyTileSelection : selectingTile) || selectingHeight) {
					int[] randIndices = null;
					int randCount = 0;
					//prebuild a list for direct array access during the random index selection
					//this could get cached but it's not
					if (isDoingNoisyTileSelection) {
						for (int i = 0; i < noiseTileCount; i++)
							randCount += noiseTileChances[i];
						randIndices = new int[randCount];
						int chance = 0;
						int maxChance = 0;
						for (int i = 0; i < noiseTileCount; i++) {
							maxChance += noiseTileChances[i];
							int tileIndex = noiseTileIndices[i];
							for (; chance < maxChance; chance++)
								randIndices[chance] = tileIndex;
						}
					}
					produceMapCoordinates(mouseX, mouseY);
					int maxX = Math.min(mouseMapX + selectedBrushSize, mapWidth - 1);
					int maxY = Math.min(mouseMapY + selectedBrushSize, mapHeight - 1);
					int minX = Math.max(mouseMapX - selectedBrushSize, 0);
					for (int y = Math.max(0, mouseMapY - selectedBrushSize); y <= maxY; y++) {
						BufferedImage[] tilesY = tiles[y];
						int[] tileIndicesY = tileIndices[y];
						int[] heightsY = heights[y];
						for (int x = minX; x <= maxX; x++) {
							if (noisyTileSelection) {
								if (isDoingNoisyTileSelection)
									tilesY[x] = tileList[tileIndicesY[x] = randIndices[(int)(Math.random() * randCount)]];
							} else if (selectingTile)
								tilesY[x] = tileList[tileIndicesY[x] = selectedTileIndex];
							if (selectingHeight)
								heightsY[x] = selectedHeight;
						}
					}
					saveSuccess = 0;
					exportSuccess = 0;
				}
			//save button
			} else if (mouseX >= 20 && mouseX < 170 && mouseY >= windowHeight + 20 && mouseY < windowHeight + 80) {
				if (saveSuccess == 0) {
					try {
						floorImage = new BufferedImage(mapWidth, mapHeight, BufferedImage.TYPE_INT_ARGB);
						for (int y = 0; y < mapHeight; y++) {
							int[] tileIndicesY = tileIndices[y];
							int[] heightsY = heights[y];
							for (int x = 0; x < mapWidth; x++) {
								if (tileIndicesY[x] == -1)
									floorImage.setRGB(x, y, 0xFFFFFFFF);
								else
									floorImage.setRGB(x, y,
										(((tileIndicesY[x] + 1) * MAP_TILES_FACTOR - 1) << 8) |
										((heightsY[x] + 1) * MAP_HEIGHTS_FACTOR - 1) |
										0xFF000000);
							}
						}
						ImageIO.write(floorImage, "png", floorImageFile);
						saveSuccess = 1;
					} catch(Exception e) {
						saveSuccess = -1;
					}
				}
			//export button
			} else if (mouseX >= 240 && mouseX < 390 && mouseY >= windowHeight + 20 && mouseY < windowHeight + 80) {
				if (exportSuccess == 0) {
					try {
						BufferedImage mapImage = new BufferedImage(
							mapWidth * TILE_SIZE, mapHeight * TILE_SIZE, BufferedImage.TYPE_INT_ARGB);
						//get the getRGB arrays for the tiles
						int[][] rgbArrays = new int[MAP_TILES][];
						for (int i = 0; i < MAP_TILES; i++)
							rgbArrays[i] = tileList[i].getRGB(0, 0, TILE_SIZE, TILE_SIZE, null, 0, TILE_SIZE);
						for (int y = 0; y < mapHeight; y++) {
							int[] tileIndicesY = tileIndices[y];
							for (int x = 0; x < mapWidth; x++) {
								int tileIndex;
								if ((tileIndex = tileIndicesY[x]) != -1)
									mapImage.setRGB(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE,
										rgbArrays[tileIndex], 0, TILE_SIZE);
							}
						}
						ImageIO.write(mapImage, "png", new File("images/map.png"));
						exportSuccess = 1;
					} catch(Exception e) {
						exportSuccess = -1;
					}
				}
			} else if (mouseX >= windowWidth + EDITOR_NOISE_BOX_X && mouseX <= windowWidth + EDITOR_NOISE_BOX_X + EDITOR_NOISE_BOX_W &&
				mouseY >= EDITOR_NOISE_BOX_Y && mouseY <= EDITOR_NOISE_BOX_Y + EDITOR_NOISE_BOX_H)
				noisyTileSelection = !noisyTileSelection;
			//one of the grid selections
			else {
				int selectedX = mouseX - windowWidth - 15;
				int selectedY = mouseY - 15;
				if (selectedX >= 0 && selectedX < 220 &&
					selectedY >= 0 && selectedY < (EDITOR_NOISE_ROW_START + EDITOR_NOISE_ROWS) * 28 - 4 &&
					selectedX % 28 < 24 && selectedY % 28 < 24) {
					int row;
					//selecting a tile
					if ((row = selectedY / 28) < EDITOR_TILE_ROWS) {
						int oldTileIndex = selectedTileIndex;
						selectedTileIndex = selectedX / 28 + row * 8;
						if (noisyTileSelection) {
							//find where the index is and increment that count
							int index = -1;
							for (int i = 0; i < noiseTileCount; i++) {
								if (noiseTileIndices[i] == selectedTileIndex) {
									index = i;
									break;
								}
							}
							if (index != -1)
								noiseTileChances[index]++;
							else if (noiseTileCount < EDITOR_NOISE_MAX_COUNT) {
								noiseTileIndices[noiseTileCount] = selectedTileIndex;
								noiseTileChances[noiseTileCount] = 1;
								noiseTileCount++;
							}
						} else
							selectingTile = (oldTileIndex == selectedTileIndex) ? !selectingTile : true;
					//selecting a height
					} else if (row >= EDITOR_HEIGHT_ROW_START && row < EDITOR_HEIGHT_ROW_START + EDITOR_HEIGHT_ROWS) {
						int oldHeight = selectedHeight;
						selectedHeight = selectedX / 28 * EDITOR_HEIGHT_ROWS + row - EDITOR_TILE_ROWS - 1;
						selectingHeight = (oldHeight == selectedHeight) ? !selectingHeight : true;
					//selecting a brush size
					} else if (row == EDITOR_BRUSH_SIZE_ROW_START)
						selectedBrushSize = selectedX / 28;
					//removing a noise
					else if (row >= EDITOR_NOISE_ROW_START && row < EDITOR_NOISE_ROW_START + EDITOR_NOISE_ROWS) {
						int index = (selectedX / 28) + (row - EDITOR_NOISE_ROW_START) * 8;
						if (index < noiseTileCount && --noiseTileChances[index] == 0) {
							for (int i = index + 1; i < noiseTileCount; i++) {
								noiseTileIndices[i - 1] = noiseTileIndices[i];
								noiseTileChances[i - 1] = noiseTileChances[i];
							}
							noiseTileCount--;
						}
					}
				}
			}
		}
		requestFocus();
	}
	public void mouseDragged(MouseEvent evt) {
		mousePressed(evt);
	}
	public void mouseMoved(MouseEvent evt) {
		int mouseX = evt.getX();
		int mouseY = evt.getY();
		if (editor) {
			if (mouseX < windowWidth && mouseY < windowHeight) {
				produceMapCoordinates(mouseX, mouseY);
			}
		}
	}
	////////////////////////////////Saving + loading////////////////////////////////
	public void save(boolean saveOptions, boolean saveGame) {
		try {
			if (saveOptions) {
				saveFile.seek(0);
				saveFile.writeInt(SAVE_FILE_VERSION);
				saveFile.writeInt(upKey);
				saveFile.writeInt(downKey);
				saveFile.writeInt(leftKey);
				saveFile.writeInt(rightKey);
				saveFile.writeInt(bootKey);
				saveFile.writeDouble(pixelWidth);
				saveFile.writeDouble(pixelHeight);
			}
			saveFile.getFD().sync();
		} catch(Exception e) {
			e.printStackTrace();
			System.exit(0);
		}
	}
	public void load(boolean loadOptions, boolean loadGame) {
		try {
			if (loadOptions) {
				saveFile.seek(0);
				int version = saveFile.readInt();
				keyStrings[0] = keyToString(upKey = saveFile.readInt());
				keyStrings[1] = keyToString(downKey = saveFile.readInt());
				keyStrings[2] = keyToString(leftKey = saveFile.readInt());
				keyStrings[3] = keyToString(rightKey = saveFile.readInt());
				keyStrings[4] = keyToString(bootKey = saveFile.readInt());
				pixelWidth = saveFile.readDouble();
				pixelHeight = saveFile.readDouble();
			}
		} catch(Exception e) {
			e.printStackTrace();
			System.exit(0);
		}
	}
	////////////////////////////////Helpers////////////////////////////////
	public void produceMapCoordinates(int mouseX, int mouseY) {
		mouseMapX = (int)(Math.floor((Math.floor(px) - BASE_WIDTH / 2 + mouseX / pixelWidth) / TILE_SIZE));
		mouseMapY = (int)(Math.floor((Math.floor(py) - BASE_HEIGHT / 2 + mouseY / pixelHeight) / TILE_SIZE));
	}
	public String keyToString(KeyEvent evt) {
		char c = evt.getKeyChar();
		if (c != KeyEvent.CHAR_UNDEFINED && c != ' ' && c != '\n') {
			if (c >= 'a' && c <= 'z')
				c += 'A' - 'a';
			return String.valueOf(c);
		} else
			return keyToString(evt.getKeyCode());
	}
	public String keyToString(int key) {
		if (key == KeyEvent.VK_UP)
			return "\u0080";
		else if (key == KeyEvent.VK_DOWN)
			return "\u0081";
		else if (key == KeyEvent.VK_LEFT)
			return "\u0082";
		else if (key == KeyEvent.VK_RIGHT)
			return "\u0083";
		else
			return KeyEvent.getKeyText(key);
	}
	////////////////////////////////Window manipulation////////////////////////////////
	public void resizeWindow(int setLocation) {
		Insets insets = window.getInsets();
		if (editor) {
			insets.right += EDITOR_MARGIN_RIGHT;
			insets.bottom += EDITOR_MARGIN_BOTTOM;
		}
		Rectangle oldBounds = window.getBounds();
		if (pixelWidth < 1.0)
			pixelWidth = 1.0;
		if (pixelHeight < 1.0)
			pixelHeight = 1.0;
		int newwidth = (windowWidth = (int)(BASE_WIDTH * pixelWidth)) + insets.left + insets.right;
		int newheight = (windowHeight = (int)(BASE_HEIGHT * pixelHeight)) + insets.top + insets.bottom;
		//1: set it to the center of the screen
		if (setLocation == 1)
			window.setBounds(
				(screenwidth() - windowWidth) / 2 - insets.left,
				(screenheight() - windowHeight) / 2 - insets.top,
				newwidth,
				newheight);
		//2: relocate according to where the window was previously, keep it centered
		else if (setLocation == 2)
			window.setBounds(
				oldBounds.x + (oldBounds.width - newwidth) / 2,
				oldBounds.y + (oldBounds.height - newheight) / 2,
				newwidth,
				newheight);
		//0: no window relocating
		else
			window.setSize(newwidth, newheight);
	}
	public void ancestorResized(HierarchyEvent evt) {
		if (evt.getID() != HierarchyEvent.ANCESTOR_RESIZED)
			return;
		Insets insets = window.getInsets();
		windowWidth = window.getWidth() - insets.left - insets.right;
		windowHeight = window.getHeight() - insets.top - insets.bottom;
		if (editor) {
			windowWidth -= EDITOR_MARGIN_RIGHT;
			windowHeight -= EDITOR_MARGIN_BOTTOM;
		}
		pixelWidth = (double)(windowWidth) / BASE_WIDTH;
		pixelHeight = (double)(windowHeight) / BASE_HEIGHT;
	}
	////////////////////////////////Unused////////////////////////////////
	public void mouseClicked(MouseEvent evt) {}
	public void mouseReleased(MouseEvent evt) {}
	public void mouseEntered(MouseEvent evt) {}
	public void mouseExited(MouseEvent evt) {}
	public void keyTyped(KeyEvent evt) {}
	public void ancestorMoved(HierarchyEvent evt) {}
}