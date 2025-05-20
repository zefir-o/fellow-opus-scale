$fn = 200;
 
height = 10;

module cup_holder()
{
    plate_thickness = 2;  
    plate_length = 126;  
    plate_width = 106;
    difference()
    {
        translate([plate_length /2 , plate_width - 38, 15])
            cylinder(h = height, d = 63 , center = false);
        translate([plate_length /2 , plate_width - 38, 15])
            cylinder(h = height, d = 52 , center = false);
        translate([29 , plate_width - 42, 15])
            cube([80, 80, height]);
    }
}


cup_holder();
