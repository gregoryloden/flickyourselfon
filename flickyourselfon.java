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
import java.awt.event.HierarchyBoundsListener;
import java.awt.event.HierarchyEvent;
import java.awt.geom.AffineTransform;
import java.awt.image.BufferedImage;
import java.io.File;
import javax.imageio.ImageIO;
import javax.swing.JFrame;
import javax.swing.JPanel;
public class flickyourselfon extends JPanel implements MouseListener, KeyListener, HierarchyBoundsListener {
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
	public static final int MAP_HEIGHTS = 8; // height = blue / 32
	public static final int MAP_TILES = 32; // tile = green / 8
	public static final int FPS = 60;
	public static final int WALK_FRAMES = 15;
	public static final double HALF_SQRT2 = Math.sqrt(2) * 0.5;
	public static final double SPEED = 0.625;
	public static final double DIAGONAL_SPEED = SPEED * HALF_SQRT2;
	//screen setup
	public double pixelWidth = 3;
	public double pixelHeight = 3;
	public int width = (int)(BASE_WIDTH * pixelWidth);
	public int height = (int)(BASE_HEIGHT * pixelHeight);
	//display
	public BufferedImage[][] tiles = null;
	public int[][] levels = null;
	public BufferedImage[][] playerSprites = null;
	public BufferedImage[] currentPlayerSprites = null;
	public int facing = 3;
	public int walkFrame = 0;
	public int mapWidth = 0;
	public int mapHeight = 0;
	public boolean painting = false;
	//game state
	public double px = 0;
	public double py = 0;
	public int vertKey = 0;
	public int horizKey = 0;
	public boolean pressedVertLast = true;
	public int upKey = KeyEvent.VK_UP;
	public int downKey = KeyEvent.VK_DOWN;
	public int leftKey = KeyEvent.VK_LEFT;
	public int rightKey = KeyEvent.VK_RIGHT;
	public int bootKey = KeyEvent.VK_SPACE;
	public static void main(String[] args) {
		try {
			JFrame window = new JFrame("Flick Yourself On");
			flickyourselfon thepanel = new flickyourselfon();
			window.setContentPane(thepanel);
			window.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
			window.setVisible(true);
			Insets insets = window.getInsets();
			window.setSize(thepanel.width + insets.left + insets.right, thepanel.height + insets.top + insets.bottom);
			window.setLocation((screenwidth() - thepanel.width) / 2 - insets.left, (screenheight() - thepanel.height) / 2 - insets.top);
			thepanel.setFocusable(true);
			thepanel.requestFocus();
			window.toFront();
			thepanel.addHierarchyBoundsListener(thepanel);
			thepanel.begin();
		} catch(Exception e) {
			e.printStackTrace();
		}
	}
	public flickyourselfon() throws Exception {
		addKeyListener(this);
		addMouseListener(this);
setBackground(Color.BLACK);
		//load all sprites
		BufferedImage player = ImageIO.read(new File("images/player.png"));
		BufferedImage tileImage = ImageIO.read(new File("images/tiles.png"));
		BufferedImage floor = ImageIO.read(new File("images/floor.png"));
		//setup the level
		//setup the player sprites
		playerSprites = new BufferedImage[4][SPRITE_FRAMES];
		for (int y = 0; y < 4; y++) {
			BufferedImage[] playerSpritesY = playerSprites[y];
			for (int x = 0; x < SPRITE_FRAMES; x++)
				playerSpritesY[x] = player.getSubimage(x * SPRITE_WIDTH, y * SPRITE_HEIGHT, SPRITE_WIDTH, SPRITE_HEIGHT);
		}
		currentPlayerSprites = playerSprites[3];
		//setup the tile list
		int tileCount = tileImage.getWidth() / TILE_SIZE;
		BufferedImage[] tileList = new BufferedImage[tileCount];
		for (int i = 0; i < tileCount; i++)
			tileList[i] = tileImage.getSubimage(i * TILE_SIZE, 0, TILE_SIZE, TILE_SIZE);
		//setup the floor map
		int heightsDivisor = 256 / MAP_HEIGHTS;
		int tilesDivisor = 256 / MAP_TILES;
		mapWidth = floor.getWidth();
		mapHeight = floor.getHeight();
		tiles = new BufferedImage[mapHeight][mapWidth];
		levels = new int[mapHeight][mapWidth];
		for (int y = 0; y < mapHeight; y++) {
			int[] levelsY = levels[y];
			BufferedImage[] tilesY = tiles[y];
			for (int x = 0; x < mapWidth; x++) {
				int pixel = floor.getRGB(x, y);
				if (pixel == 0) {
					levelsY[x] = -1;
					tilesY[x] = null;
				} else {
					levelsY[x] = (pixel & 255) / heightsDivisor;
					tilesY[x] = tileList[((pixel >> 8) & 255) / tilesDivisor];
				}
			}
		}
	}
	////////////////////////////////Main loop////////////////////////////////
	public void begin() throws Exception {
		long now = System.currentTimeMillis();
		int next = 1;
		while (true) {
			while (System.currentTimeMillis() - now < (next * 1000 / FPS))
				Thread.sleep(1);
			update();
			if (!painting)
				repaint();
			next = next % FPS + 1;
			if (next == 1)
				now = System.currentTimeMillis();
		}
	}
	////////////////////////////////Draw////////////////////////////////
	public void paintComponent(Graphics g) {
		painting = true;
super.paintComponent(g);
		Graphics2D g2 = (Graphics2D)(g);
		g2.transform(AffineTransform.getScaleInstance(pixelWidth, pixelHeight));
		drawFloor(g);
		int spriteFrame = walkFrame / WALK_FRAMES % 4;
		g.drawImage(currentPlayerSprites[spriteFrame < 2 ? spriteFrame : (spriteFrame - 2) << 1],
			(BASE_WIDTH - SPRITE_WIDTH) / 2, (BASE_HEIGHT - SPRITE_HEIGHT) / 2, null);
		g.setColor(Color.WHITE);
		g.setFont(new Font("Monospace", Font.BOLD, 8));
		g.drawString("W: " + String.format("%.3f", pixelWidth), 30, 30);
		g.drawString("H: " + String.format("%.3f", pixelHeight), 30, 40);
		painting = false;
	}
	public void drawFloor(Graphics g) {
		double addX = (BASE_WIDTH / 2) - px;
		double addY = (BASE_HEIGHT / 2) - py;
		for (int y = 0; y < mapHeight; y++) {
			BufferedImage[] tilesY = tiles[y];
			for (int x = 0; x < mapWidth; x++)
				g.drawImage(tilesY[x], (int)(x * TILE_SIZE + addX), (int)(y * TILE_SIZE + addY), null);
		}
	}
	////////////////////////////////Update////////////////////////////////
	public void update() {
		//move sqrt2/2 in each direction if both directions are pressed
		boolean bothPressed = ((vertKey & horizKey) != 0);
		double dist = bothPressed ? DIAGONAL_SPEED : SPEED;
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
		}
	}
	public void mousePressed(MouseEvent evt) {
		requestFocus();
	}
	public void ancestorResized(HierarchyEvent evt) {
		if (evt.getID() != HierarchyEvent.ANCESTOR_RESIZED)
			return;
		JFrame window = (JFrame)(getTopLevelAncestor());
		Insets insets = window.getInsets();
		width = window.getWidth() - insets.left - insets.right;
		height = window.getHeight() - insets.top - insets.bottom;
		pixelWidth = (double)(width) / BASE_WIDTH;
		pixelHeight = (double)(height) / BASE_HEIGHT;
	}
	public void mouseClicked(MouseEvent evt) {}
	public void mouseReleased(MouseEvent evt) {}
	public void mouseEntered(MouseEvent evt) {}
	public void mouseExited(MouseEvent evt) {}
	public void keyTyped(KeyEvent evt) {}
	public void ancestorMoved(HierarchyEvent evt) {}
}