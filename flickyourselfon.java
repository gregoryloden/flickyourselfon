// Designed 2014-10-11
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Insets;
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
	public static final int SPRITE_WIDTH = 9;
	public static final int SPRITE_HEIGHT = 19;
	public static final int SPRITE_FRAMES = 3;
	public static final int TILE_SIZE = 6;
	public static final int MAP_HEIGHTS = 16; // height = blue / 16
	public static final int MAP_TILES = 64; // tile = green / 8
	public static final int MAP_HEIGHTS_FACTOR = 256 / MAP_HEIGHTS;
	public static final int MAP_TILES_FACTOR = 256 / MAP_TILES;
	public static final int FPS = 60;
	public static final int WALK_FRAMES = 15;
	public static final double HALF_SQRT2 = Math.sqrt(2) * 0.5;
	public static final double SPEED = 0.625;
	public static final double DIAGONAL_SPEED = SPEED * HALF_SQRT2;
	public static final double STARTING_PLAYER_X = 198.0;
	public static final double STARTING_PLAYER_Y = 166.0;
	public static File playerImageFile = new File("images/player.png");
	public static File tilesImageFile = new File("images/tiles.png");
	public static File floorImageFile = new File("images/floor.png");
	//function outputs
	public static int mapX = 0;
	public static int mapY = 0;
	//screen setup
	public double pixelWidth = 3.0;
	public double pixelHeight = 3.0;
	public int width = (int)(BASE_WIDTH * pixelWidth);
	public int height = (int)(BASE_HEIGHT * pixelHeight);
	//display
	public BufferedImage playerImage = null;
	public BufferedImage tilesImage = null;
	public BufferedImage floorImage = null;
	public BufferedImage[] tileList = new BufferedImage[MAP_TILES];
	public BufferedImage[][] tiles = null;
	public int[][] tileIndices = null;
	public int[][] heights = null;
	public BufferedImage[][] playerSprites = new BufferedImage[4][SPRITE_FRAMES];
	public BufferedImage[] currentPlayerSprites = null;
	public int facing = 3;
	public int walkFrame = 0;
	public int mapWidth = 0;
	public int mapHeight = 0;
	public boolean painting = false;
	//game state
	public double px = STARTING_PLAYER_X;
	public double py = STARTING_PLAYER_Y;
	public int vertKey = 0;
	public int horizKey = 0;
	public boolean pressedVertLast = true;
	public boolean bootKeyPressed = false;
	public int upKey = KeyEvent.VK_UP;
	public int downKey = KeyEvent.VK_DOWN;
	public int leftKey = KeyEvent.VK_LEFT;
	public int rightKey = KeyEvent.VK_RIGHT;
	public int bootKey = KeyEvent.VK_SPACE;
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
	public int saveSuccess = 0;
	public int exportSuccess = 0;
	public static void main(String[] args) {
		try {
			//initialize game
			flickyourselfon thepanel = new flickyourselfon();
			for (int i = 0; i < args.length; i++) {
				if (args[i].equals("-editor"))
					thepanel.editor = true;
/*
else if (args[i].equals("-dothing")) {
	BufferedImage image = ImageIO.read(floorImageFile);
	int[] rgbs = image.getRGB(0, 0, image.getWidth(), image.getHeight(), null, 0, image.getWidth());
	for (int j = 0; j < rgbs.length; j++) {
		int pixel = rgbs[j];
		int tilenum = ((pixel >> 8) & 255) / MAP_TILES_FACTOR;
		if (pixel != 0) {
//VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
	tilenum = (tilenum - 1) / 2;
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
			rgbs[j] = (pixel & 0xFFFF00FF) | (((tilenum + 1) * MAP_TILES_FACTOR - 1) << 8);
		}
	}
	image.setRGB(0, 0, image.getWidth(), image.getHeight(), rgbs, 0, image.getWidth());
	ImageIO.write(image, "png", floorImageFile);
}
/**/
			}
			thepanel.resetGame();
			//initialize display window
			JFrame window = new JFrame("Flick Yourself On");
			window.setContentPane(thepanel);
			window.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
			window.setVisible(true);
			Insets insets = window.getInsets();
			if (thepanel.editor) {
				insets.right += EDITOR_MARGIN_RIGHT;
				insets.bottom += EDITOR_MARGIN_BOTTOM;
			}
			window.setSize(thepanel.width + insets.left + insets.right, thepanel.height + insets.top + insets.bottom);
			window.setLocation((screenwidth() - thepanel.width) / 2 - insets.left, (screenheight() - thepanel.height) / 2 - insets.top);
			thepanel.setFocusable(true);
			thepanel.requestFocus();
			window.toFront();
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
	public flickyourselfon() {
		addKeyListener(this);
		addMouseListener(this);
		addMouseMotionListener(this);
setBackground(Color.BLACK);
	}
	public void resetGame() throws Exception {
		//load all sprites
		playerImage = ImageIO.read(playerImageFile);
		tilesImage = ImageIO.read(tilesImageFile);
		floorImage = ImageIO.read(floorImageFile);
		//setup the level
		//setup the player sprites
		for (int y = 0; y < 4; y++) {
			BufferedImage[] playerSpritesY = playerSprites[y];
			for (int x = 0; x < SPRITE_FRAMES; x++)
				playerSpritesY[x] = playerImage.getSubimage(x * SPRITE_WIDTH, y * SPRITE_HEIGHT, SPRITE_WIDTH, SPRITE_HEIGHT);
		}
		currentPlayerSprites = playerSprites[3];
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
				if (pixel == 0) {
					heightsY[x] = -1;
					tilesY[x] = null;
					tileIndicesY[x] = -1;
				} else {
					heightsY[x] = (pixel & 255) / MAP_HEIGHTS_FACTOR;
					tilesY[x] = tileList[tileIndicesY[x] = ((pixel >> 8) & 255) / MAP_TILES_FACTOR];
				}
			}
		}
		//setup game state
		px = STARTING_PLAYER_X;
		py = STARTING_PLAYER_Y;
		facing = 3;
		vertKey = 0;
		horizKey = 0;
		pressedVertLast = true;
		bootKeyPressed = false;
	}
	////////////////////////////////Draw////////////////////////////////
	public void paintComponent(Graphics g) {
super.paintComponent(g);
		Graphics2D g2 = (Graphics2D)(g);
		AffineTransform oldTransform = g2.getTransform();
		g2.transform(AffineTransform.getScaleInstance(pixelWidth, pixelHeight));
		drawFloor(g);
		int spriteFrame = walkFrame / WALK_FRAMES % 4;
		g.drawImage(currentPlayerSprites[spriteFrame < 2 ? spriteFrame : (spriteFrame - 2) << 1],
			(BASE_WIDTH - SPRITE_WIDTH) / 2, (BASE_HEIGHT - SPRITE_HEIGHT) / 2, null);
//g.setColor(Color.WHITE);
//g.setFont(new Font("Dialog", Font.BOLD, 8));
//g.drawString("W: " + String.format("%.3f", pixelWidth), 30, 30);
//g.drawString("H: " + String.format("%.3f", pixelHeight), 30, 40);
		if (editor) {
			//hover tint
			if (selectingTile || selectingHeight || (noisyTileSelection && noiseTileCount > 0)) {
				int dx = (int)(BASE_WIDTH / 2 - px) + (mapX - selectedBrushSize) * TILE_SIZE;
				int dy = (int)(BASE_HEIGHT / 2 - py) + (mapY - selectedBrushSize) * TILE_SIZE;
				int drawSize = ((selectedBrushSize << 1) + 1) * TILE_SIZE;
				g.setColor(Color.BLACK);
				g.drawRect(dx - 1, dy - 1, drawSize + 1, drawSize + 1);
				g.setColor(Color.WHITE);
				g.drawRect(dx - 2, dy - 2, drawSize + 3, drawSize + 3);
			}
			//blue backgrounds
			g2.setTransform(oldTransform);
			g.setColor(editorBlue);
			g.fillRect(width, 0, EDITOR_MARGIN_RIGHT, height);
			g.fillRect(0, height, width + EDITOR_MARGIN_RIGHT, EDITOR_MARGIN_BOTTOM);
			//tile images
			int leftX = width + 15;
			for (int y = 0; y < EDITOR_TILE_ROWS; y++) {
				int dy = 15 + y * 28;
				for (int x = 0; x < 8; x++) {
					int index = x + y * 8;
					int dx = leftX + x * 28;
					g.drawImage(tileList[index], dx, dy, 24, 24, null);
					g.setColor((selectingTile && index == selectedTileIndex && !noisyTileSelection) ? editorYellow : editorGray);
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
					g.setColor((selectingHeight && heightNum == selectedHeight) ? editorYellow : editorGray);
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
			g.drawString("Noise", width + EDITOR_NOISE_BOX_X + 2, EDITOR_NOISE_BOX_Y + 12);
			g.setColor(noisyTileSelection ? editorYellow : editorGray);
			g.drawRect(width + EDITOR_NOISE_BOX_X, EDITOR_NOISE_BOX_Y, EDITOR_NOISE_BOX_W, EDITOR_NOISE_BOX_H);
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
			g.drawRect(20, height + 20, 149, 59);
			g.drawRect(21, height + 21, 147, 57);
			g.fillRect(24, height + 24, 142, 52);
			g.setColor(editorSaveGreen);
			g.setFont(saveFont);
			g.drawString("Save", 43, height + 63);
			g.setColor(Color.WHITE);
			g.fillRect(181, height + 31, 38, 38);
			g.setColor(editorGray);
			g.drawRect(180, height + 30, 39, 39);
			if (saveSuccess == 1) {
				g.setColor(editorSaveSuccessGreen);
				g.drawString("\u2713", 182, height + 65);
			} else if (saveSuccess == -1) {
				g.setColor(editorSaveSuccessRed);
				g.drawString("\u2715", 179, height + 65);
			}
			//export map button
			g.setColor(editorExportGreen);
			g.drawRect(240, height + 20, 149, 59);
			g.drawRect(241, height + 21, 147, 57);
			g.fillRect(244, height + 24, 142, 52);
			g.setColor(editorExportRed);
			g.setFont(exportFont);
			g.drawString("Export Map", 250, height + 56);
			g.setColor(Color.WHITE);
			g.fillRect(401, height + 31, 38, 38);
			g.setColor(editorGray);
			g.drawRect(400, height + 30, 39, 39);
			if (exportSuccess == 1) {
				g.setFont(saveFont);
				g.setColor(editorSaveSuccessGreen);
				g.drawString("\u2713", 402, height + 65);
			} else if (exportSuccess == -1) {
				g.setFont(saveFont);
				g.setColor(editorSaveSuccessRed);
				g.drawString("\u2715", 399, height + 65);
			}
		}
		painting = false;
	}
	public void drawFloor(Graphics g) {
		int addX = (int)(BASE_WIDTH / 2 - px);
		int addY = (int)(BASE_HEIGHT / 2 - py);
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
	////////////////////////////////Update////////////////////////////////
	public void update() {
		//move sqrt2/2 in each direction if both directions are pressed
		boolean bothPressed = ((vertKey & horizKey) != 0);
		double dist = bothPressed ? DIAGONAL_SPEED : SPEED;
		if (editor && bootKeyPressed)
			dist *= EDITOR_SPEED_BOOST;
		int newFacing = -1;
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
		if (newFacing != -1) {
			currentPlayerSprites = playerSprites[newFacing];
			facing = newFacing;
		}
		//update the animation frame
		if ((vertKey | horizKey) != 0)
			walkFrame += 1;
		else
			walkFrame = 0;
	}
	////////////////////////////////Input////////////////////////////////
	public void keyPressed(KeyEvent evt) {
		int code = evt.getKeyCode();
		if (code == upKey) {
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
		} else if (code == bootKey)
			bootKeyPressed = true;
		else if (code == KeyEvent.VK_R) {
			try {
				resetGame();
			} catch(Exception e) {
			}
		}
	}
	public void keyReleased(KeyEvent evt) {
		int code = evt.getKeyCode();
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
		} else if (code == bootKey)
			bootKeyPressed = false;
	}
	public void mousePressed(MouseEvent evt) {
		int mouseX = evt.getX();
		int mouseY = evt.getY();
		if (editor) {
			//paint a tile
			if (mouseX < width && mouseY < height) {
				if (selectingTile || selectingHeight || (noisyTileSelection && noiseTileCount > 0)) {
					produceMapCoordinates(mouseX, mouseY);
					int maxX = Math.min(mapX + selectedBrushSize, mapWidth - 1);
					int maxY = Math.min(mapY + selectedBrushSize, mapHeight - 1);
					int minX = Math.max(mapX - selectedBrushSize, 0);
					int[] randIndices = null;
					int randCount = 0;
					//prebuild a list for direct array access during the random index selection
					if (noisyTileSelection) {
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
					for (int y = Math.max(0, mapY - selectedBrushSize); y <= maxY; y++) {
						BufferedImage[] tilesY = tiles[y];
						int[] tileIndicesY = tileIndices[y];
						int[] heightsY = heights[y];
						for (int x = minX; x <= maxX; x++) {
							if (heightsY[x] != -1 || ((noisyTileSelection || selectingTile) && selectingHeight)) {
								if (noisyTileSelection)
									tilesY[x] = tileList[tileIndicesY[x] = randIndices[(int)(Math.random() * randCount)]];
								else if (selectingTile)
									tilesY[x] = tileList[tileIndicesY[x] = selectedTileIndex];
								if (selectingHeight)
									heightsY[x] = selectedHeight;
							}
						}
					}
					saveSuccess = 0;
					exportSuccess = 0;
				}
			//save button
			} else if (mouseX >= 20 && mouseX < 170 && mouseY >= height + 20 && mouseY < height + 80) {
				if (saveSuccess == 0) {
					try {
						floorImage = new BufferedImage(mapWidth, mapHeight, BufferedImage.TYPE_INT_ARGB);
						for (int y = 0; y < mapHeight; y++) {
							int[] tileIndicesY = tileIndices[y];
							int[] heightsY = heights[y];
							for (int x = 0; x < mapWidth; x++) {
								if (heightsY[x] != -1)
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
			} else if (mouseX >= 240 && mouseX < 390 && mouseY >= height + 20 && mouseY < height + 80) {
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
			} else if (mouseX >= width + EDITOR_NOISE_BOX_X && mouseX <= width + EDITOR_NOISE_BOX_X + EDITOR_NOISE_BOX_W &&
				mouseY >= EDITOR_NOISE_BOX_Y && mouseY <= EDITOR_NOISE_BOX_Y + EDITOR_NOISE_BOX_H)
				noisyTileSelection = !noisyTileSelection;
			//one of the grid selections
			else {
				int selectedX = mouseX - width - 15;
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
			if (mouseX < width && mouseY < height) {
				produceMapCoordinates(mouseX, mouseY);
			}
		}
	}
	////////////////////////////////Helpers////////////////////////////////
	public void produceMapCoordinates(int mouseX, int mouseY) {
		mapX = ((int)(px - BASE_WIDTH / 2) + (int)(mouseX / pixelWidth)) / TILE_SIZE;
		mapY = ((int)(py - BASE_HEIGHT / 2) + (int)(mouseY / pixelHeight)) / TILE_SIZE;
	}
	////////////////////////////////Other////////////////////////////////
	public void ancestorResized(HierarchyEvent evt) {
		if (evt.getID() != HierarchyEvent.ANCESTOR_RESIZED)
			return;
		JFrame window = (JFrame)(getTopLevelAncestor());
		Insets insets = window.getInsets();
		width = window.getWidth() - insets.left - insets.right;
		height = window.getHeight() - insets.top - insets.bottom;
		if (editor) {
			width -= EDITOR_MARGIN_RIGHT;
			height -= EDITOR_MARGIN_BOTTOM;
		}
		pixelWidth = (double)(width) / BASE_WIDTH;
		pixelHeight = (double)(height) / BASE_HEIGHT;
	}
	////////////////////////////////Unused////////////////////////////////
	public void mouseClicked(MouseEvent evt) {}
	public void mouseReleased(MouseEvent evt) {}
	public void mouseEntered(MouseEvent evt) {}
	public void mouseExited(MouseEvent evt) {}
	public void keyTyped(KeyEvent evt) {}
	public void ancestorMoved(HierarchyEvent evt) {}
}