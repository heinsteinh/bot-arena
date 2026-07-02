PShape s;
float x = 0;
float y = 0; 
float z = 0;

float vx = 0;
float vy = 0;
float vz = 0;

float rx = 0;
float ry = 0;
float rz = 0;

float shotspeed = -100;

boolean wP = false;
boolean aP = false;
boolean sP = false;
boolean dP = false;
boolean qP = false;
boolean eP = false;

float acceleration = 0.4;

ArrayList<Shot> shots;


float myDist = 100;
float speed = 4;

class Shot {
  public float x, y, z, vx, vy, vz;
  public float rx, ry, rz;
  
  Shot(float x, float y, float z, float vx, float vy, float vz, float rx, float ry, float rz) {
    this.x = x;
    this.y = y;
    this.z = z;
    this.vx = vx;
    this.vy = vy;
    this.vz = vz;
    this.rx = rx;
    this.ry = ry;
    this.rz = rz;
  }
  
  void move() {
    this.x += this.vx;
    this.y += this.vy;
    this.z += this.vz;
  }
  
  void show() {
    noStroke();
    fill(150,150,255);
    
    pushMatrix();
    
    translate(this.x,this.y,this.z);
    
    //rotateX(this.ry);
    //rotateY(this.rx);
    //rotateZ(this.rx);

    scale(1,1,20);
    
    box(10);
    
    popMatrix();
  }
}


void setup() {
  fullScreen(P3D);
  s = loadShape("Spaceship3.obj");
  shots = new ArrayList<Shot>();
}

void draw() {
  
  // Events
  if (wP) { vy -= acceleration;}
  if (sP) {vy += acceleration;}
  
  if (aP) {vx -= acceleration;}
  if (dP) {vx += acceleration;}
  
  if (qP) {rz -= 0.01;}
  if (eP) {rz += 0.01;}
  
  //Physics
  x += vx;
  y += vy;
  z += vz;
  
  vx *= 0.97;
  vy *= 0.97;
  vz *= 0.97;
  
  rx = 0.5*y/height;
  ry = 0.5*x/width;
  rz = 0.5*x/width;
  
  for (Shot shot : shots) {shot.move();}
  
  for (int i = shots.size() - 1; i >= 0; i--) {
    if (abs(shots.get(i).z) > 100000) {
      shots.remove(i);
    }
  }
  
  //Render
  
  background(10);
  ambientLight(150, 150, 150);
  //directionalLight(206, 206, 206, 0, -0.2, 1);
  directionalLight(206,206,206, 0.1, -0.3, 0.5);
  
  
  noFill();
  stroke(0,0,255);
  strokeWeight(2);
  
  for (float z = -10000 + (speed*frameCount) % myDist; z < 1000; z += myDist) {
    pushMatrix();
    stroke(0,0,255+z/20);
    translate(width/2, height/2, z);
    rotate(z / 4000f); // + frameCount / -100.0
    float angle = TWO_PI / 6;
    for (float a = 0; a < TWO_PI; a += angle) {
      float sx = cos(a) * 1200;
      float sy = sin(a) * 1200;
      line(sx, sy, 0, sx, sy, myDist);
    }
    polygon(0, 0, 1200, 6);
    popMatrix();
  }
  
  
  translate(width/2, height/2,-300);
 
  for (Shot shot : shots) {shot.show();}
 
  pushMatrix();
  translate(x,y,z);
  rotateY(PI);
  rotateX(rx);
  rotateY(ry);
  rotateZ(rz);
  scale(50,-50,50);
  shape(s,0,0);
  popMatrix();
  

}






void polygon(float x, float y, float radius, int npoints) {
  float angle = TWO_PI / npoints;
  beginShape();
  for (float a = 0; a < TWO_PI; a += angle) {
    float sx = x + cos(a) * radius;
    float sy = y + sin(a) * radius;
    vertex(sx, sy);
  }
  endShape(CLOSE);
}

void keyPressed() {
  if (key == 'w') {wP = true;}
  if (key == 'a') {aP = true;}
  if (key == 's') {sP = true;}
  if (key == 'd') {dP = true;}
  if (key == 'q') {qP = true;}
  if (key == 'e') {eP = true;}
  if (key == ' ') {
    shots.add(new Shot(x-150,y,z+20, 0,0,shotspeed, rx, ry, rz));
    shots.add(new Shot(x-100,y,z+20, 0,0,shotspeed, rx, ry, rz));
    shots.add(new Shot(x-50,y,z+20, 0,0,shotspeed, rx, ry, rz));
    shots.add(new Shot(x+50,y,z+20, 0,0,shotspeed, rx, ry, rz));
    shots.add(new Shot(x+100,y,z+20, 0,0,shotspeed, rx, ry, rz));
    shots.add(new Shot(x+150,y,z+20, 0,0,shotspeed, rx, ry, rz));
  }
}

void keyReleased() {
  if (key == 'w') {wP = false;}
  if (key == 'a') {aP = false;}
  if (key == 's') {sP = false;}
  if (key == 'd') {dP = false;}
  if (key == 'q') {qP = false;}
  if (key == 'e') {eP = false;}

}
