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
	public static final int BASE_WIDTH = 200;
	public static final int BASE_HEIGHT = 150;
	public double pixelwidth = 3;
	public double pixelheight = 3;
	public int width = (int)(BASE_WIDTH * pixelwidth);
	public int height = (int)(BASE_HEIGHT * pixelheight);
	public BufferedImage player = null;
	public int px;
	public int py;
	public BufferedImage walls = null;
	public BufferedImage floor = null;
	public boolean readytoresize = false;
	public static void main(String[] args) {
		JFrame window = new JFrame("Flick Yourself On");
		flickyourselfon thepanel = new flickyourselfon();
		window.setContentPane(thepanel);
		window.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		window.setVisible(true);
		Insets insets = window.getInsets();
		window.setSize(thepanel.width + insets.left + insets.right, thepanel.height + insets.top + insets.bottom);
		window.setLocation((screenwidth() - thepanel.width) / 2 - insets.left, (screenheight() - thepanel.height) / 2 - insets.top);
		thepanel.setFocusable(true);
		window.toFront();
		thepanel.readytoresize = true;
	}
	public flickyourselfon() {
		addKeyListener(this);
		addMouseListener(this);
		addHierarchyBoundsListener(this);
setBackground(Color.BLACK);
		//load all sprites
		try {
			player = ImageIO.read(new File("images/player.png"));
			walls = ImageIO.read(new File("images/walls.png"));
			floor = ImageIO.read(new File("images/floor.png"));
		} catch(Exception e) {
		}
	}
	public void paintComponent(Graphics g) {
super.paintComponent(g);
		Graphics2D g2 = (Graphics2D)(g);
		g2.setTransform(AffineTransform.getScaleInstance(pixelwidth, pixelheight));
		g.drawImage(floor, 0, 0, null);
		g.drawImage(player, 100, 75, null);
		g.setColor(Color.WHITE);
		g.setFont(new Font("Monospace", Font.BOLD, 8));
		g.drawString("W: " + String.format("%.3f", pixelwidth), 30, 30);
		g.drawString("H: " + String.format("%.3f", pixelheight), 30, 40);
	}
	public void mousePressed(MouseEvent evt) {
		repaint();
	}
	public void ancestorResized(HierarchyEvent evt) {
		if (!readytoresize || evt.getID() != HierarchyEvent.ANCESTOR_RESIZED)
			return;
		JFrame window = (JFrame)(getTopLevelAncestor());
		Insets insets = window.getInsets();
		width = window.getWidth() - insets.left - insets.right;
		height = window.getHeight() - insets.top - insets.bottom;
		pixelwidth = (double)(width) / BASE_WIDTH;
		pixelheight = (double)(height) / BASE_HEIGHT;
		repaint();
	}
	public void mouseClicked(MouseEvent evt) {}
	public void mouseReleased(MouseEvent evt) {}
	public void mouseEntered(MouseEvent evt) {}
	public void mouseExited(MouseEvent evt) {}
	public void keyTyped(KeyEvent evt) {}
	public void keyPressed(KeyEvent evt) {}
	public void keyReleased(KeyEvent evt) {}
	public void ancestorMoved(HierarchyEvent evt) {}
}