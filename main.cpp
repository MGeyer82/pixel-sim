#include <iostream>
#include <SFML/Window.hpp>
#include <SFML/Main.hpp>
#include <SFML/Graphics.hpp>

#define RGBA_SIZE 4	// each RGBA pixel buffer 'index' consists of 4 uint8_t values (R, G, B, A)

//TODO: Next steps:
//	- Change color of particles drawn via button press
//	- Make particles fall "naturally" (not just in a straight line)
//	- Add different types of particles (much more involved change)

// Screen dimensions and cell size in pixels
unsigned int screen_width = 800;
unsigned int screen_height = 600;
int cell_size = 5; // square cells, so only one dimension is needed

// Divide the window into a grid consisting of 5x5 pixel squares
int grid_row_size = screen_width / cell_size;  // 160 (159 w/ zero index)
int grid_col_size = screen_height / cell_size; // 120 (119 w/ zero index)

// Grid and pixels describe the screen space in different ways 
// (grid: 5x5 pixel cells, pixels: individual RGBA values for each pixel), but they are kept in sync by 
// the drawCellToBuffer and clearCellFromBuffer functions
std::vector<int> grid{ grid_row_size * grid_col_size, 0 };			 // 2D grid represented as a 1D vector
std::vector<uint8_t> pixels(screen_width * screen_height * RGBA_SIZE, 0); // RGBA pixel buffer for the entire screen

// Helper function to easily access grid cells using x and y coordinates
inline int& cell(int x, int y)
{
	return grid[y * grid_row_size + x];
}

// Set the passed RGBA values for the pixel at the specified coordinates
inline void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	int index = (y * screen_width + x) * RGBA_SIZE; // Calculate the index for RGBA buffer
	pixels[index] = r;     // Red
	pixels[index + 1] = g; // Green
	pixels[index + 2] = b; // Blue
	pixels[index + 3] = a; // Alpha
}

// add pixels that make up a cell of the grid to the texture buffer
void drawCellToBuffer(int grid_x, int grid_y)
{
	int start_x = grid_x * cell_size;
	int start_y = grid_y * cell_size;

	for (int dy = 0; dy < cell_size; ++dy)
	{
		for (int dx = 0; dx < cell_size; ++dx)
		{
			int px = start_x + dx;
			int py = start_y + dy;

			setPixel(px, py, 255, 255, 255, 255); // white sand
		}
	}
}

// clear pixels that make up a cell of the grid from the texture buffer
void clearCellFromBuffer(int grid_x, int grid_y)
{
	int start_x = grid_x * cell_size;
	int start_y = grid_y * cell_size;

	for (int dy = 0; dy < cell_size; ++dy)
	{
		for (int dx = 0; dx < cell_size; ++dx)
		{
			int px = start_x + dx;
			int py = start_y + dy;

			setPixel(px, py, 0, 0, 0, 255); // black background
		}
	}
}

// Currently draws a single cell to the screen
void drawScreen(sf::RenderWindow& window, sf::Texture& texture, sf::Sprite& sprite)
{
	texture.update(pixels.data());

	window.clear();
	window.draw(sprite);
	window.display();
}

bool validGridRange(int x_pos, int y_pos)
{
	if ((x_pos < 0 || x_pos >= (grid_row_size)) ||
		(y_pos < 0 || y_pos >= (grid_col_size)))
	{

		return false;
	}

	return true;
}

bool emptyGridSpace(int x_index, int y_index)
{
	if (cell(x_index, y_index) == 0) return true;
	return false;
}

bool validPositionRange(sf::Vector2i& mouse_pos)
{
	if ((mouse_pos.x < 0 || mouse_pos.x >= screen_width) ||
		(mouse_pos.y < 0 || mouse_pos.y >= screen_height))
	{

		return false;
	}

	return true;
}

void updatePositions(std::vector<sf::Vector2i>& active, std::vector<sf::Vector2i>& next_active)
{
	next_active.clear();

	// iterate bottom to top to prevent multiple position updates per frame
	for (auto& p : active)
	{
		if (cell(p.x, p.y) == 0) continue;

		bool moved = false;

		// Falling down - check the space directly below the current pixel
		if (!validGridRange(p.x, p.y + 1))
		{
			next_active.push_back(p);
			continue;
		}

		// Unoccupied space below - move down
		if (emptyGridSpace(p.x, p.y + 1))
		{
			// Clear old position (visual)
			clearCellFromBuffer(p.x, p.y);

			// Update grid (logic)
			cell(p.x, p.y) = 0;
			cell(p.x, p.y + 1) = 1;

			// Draw new position (visual)
			drawCellToBuffer(p.x, p.y + 1);
			next_active.push_back({ p.x, p.y + 1 });
			moved = true;
		}
		// Occupied space below - check down-left and down-right
		else
		{
			bool down_left_empty = false;
			bool down_right_empty = false;

			// Check down-left
			if (validGridRange(p.x - 1, p.y + 1) && emptyGridSpace(p.x - 1, p.y + 1))
			{
				down_left_empty = true;
			}

			// Check down-right
			if (validGridRange(p.x + 1, p.y + 1) && emptyGridSpace(p.x + 1, p.y + 1))
			{
				down_right_empty = true;
			}

			// If both unoccupied, randomly select one to move into
			if (down_left_empty && down_right_empty)
			{
				if (rand() % 2 == 0)
				{
					// fill down_left
					clearCellFromBuffer(p.x, p.y);

					cell(p.x, p.y) = 0;
					cell(p.x - 1, p.y + 1) = 1;

					drawCellToBuffer(p.x - 1, p.y + 1);
					next_active.push_back({ p.x - 1, p.y + 1 });
					moved = true;
				}
				else
				{
					// fill down_right
					clearCellFromBuffer(p.x, p.y);

					cell(p.x, p.y) = 0;
					cell(p.x + 1, p.y + 1) = 1;

					drawCellToBuffer(p.x + 1, p.y + 1);
					next_active.push_back({ p.x + 1, p.y + 1 }); // add new position to active list for next frame
					moved = true;
				}
			}
			// If one is occupied and the other is unoccupied, move into the unoccupied space
			else if (!down_left_empty && down_right_empty)
			{
				// fill down_right
				clearCellFromBuffer(p.x, p.y);

				cell(p.x, p.y) = 0;
				cell(p.x + 1, p.y + 1) = 1;

				drawCellToBuffer(p.x + 1, p.y + 1);
				next_active.push_back({ p.x + 1, p.y + 1 });
				moved = true;
			}
			else if (down_left_empty && !down_right_empty)
			{
				// fill down_left
				clearCellFromBuffer(p.x, p.y);

				cell(p.x, p.y) = 0;
				cell(p.x - 1, p.y + 1) = 1;

				drawCellToBuffer(p.x - 1, p.y + 1);
				next_active.push_back({ p.x - 1, p.y + 1 });
				moved = true;
			}
		}

		if (!moved)
		{
			next_active.push_back(p);
		}
	}

	active.swap(next_active);
}

// While click held...
void addPixels(sf::RenderWindow& window, std::vector<sf::Vector2i>& active)
{
	// first get the current position of the mouse relative to the window...
	sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);

	if (!validPositionRange(mouse_pos)) return;

	// then use integer division to truncate (floor), then multiply by cell size to snap to pixel coordinates
	sf::Vector2i grid_pos = (mouse_pos / cell_size);

	// Set the corresponding grid cell to 1 (indicating it's active)
	cell(grid_pos.x, grid_pos.y) = 1;
	drawCellToBuffer(grid_pos.x, grid_pos.y);
	active.push_back({ grid_pos.x, grid_pos.y });

}

void handleEvents(sf::RenderWindow& window)
{
	while (const std::optional event = window.pollEvent())
	{
		// 'getIf' returns a pointer to the event subtype.
		// 'is' returns true if event subtype matches the template parameter, false otherwise.

		// Close the window
		if (event->is<sf::Event::Closed>())
			window.close();
	}
}

int main()
{
	// Set the dimensions, title, and create the window
	sf::RenderWindow window(sf::VideoMode({ (unsigned)screen_width, (unsigned)screen_height }), "Sand Simulator");
	window.setFramerateLimit(60);

	sf::Vector2u size({ screen_width, screen_height });
	sf::Texture texture(size);
	sf::Sprite sprite(texture);

	std::vector<sf::Vector2i> active_cells;		  // Store the positions of active cells for optimized updates
	std::vector<sf::Vector2i> next_active_cells;  // Store the positions of cells that will become active in the next frame

	cell(80, 10) = 1;
	drawCellToBuffer(80, 10);

	// master loop
	while (window.isOpen())
	{
		// master event handler - calls event functions as needed
		handleEvents(window);
		updatePositions(active_cells, next_active_cells);
		drawScreen(window, texture, sprite);

		// handle inputs
		if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
		{
			addPixels(window, active_cells);
		}
	}

	return 0;
}