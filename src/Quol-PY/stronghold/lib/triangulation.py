import math
import re


class StrongholdTriangulator:
    def __init__(self):
        self.throws = set()
        self.insert_f3c_string('/execute in minecraft:overworld run tp @s 503.94 63.00 -1344.39 -632.43 -28.46')
        self.insert_f3c_string('/execute in minecraft:overworld run tp @s 1015.62 71.00 -1288.43 -989.42 -28.83')

    def insert(self, x, y, z, yaw, pitch):
        self.throws.add((float(x), float(z), float(yaw)))

    def insert_f3c_string(self, f3c_string):
        numbers = re.findall(r"-?\d+\.\d+", f3c_string)

        if len(numbers) < 5:
            return

        x = float(numbers[0])
        y = float(numbers[1])
        z = float(numbers[2])
        yaw = float(numbers[3])
        pitch = float(numbers[4])

        self.insert(x, y, z, yaw, pitch)

    def clear(self):
        self.throws.clear()

    @staticmethod
    def _yaw_to_direction(yaw_deg):
        yaw_rad = math.radians(yaw_deg)
        dx = -math.sin(yaw_rad)
        dz = math.cos(yaw_rad)
        return dx, dz

    @staticmethod
    def _solve_2x2(a, b, c, d, e, f):
        """
        Solves:
            a*x + b*z = e
            c*x + d*z = f
        """
        det = a * d - b * c

        if abs(det) < 1e-10:
            return None, None

        x = (e * d - b * f) / det
        z = (a * f - e * c) / det

        return x, z

    def calculate(self):
        if len(self.throws) < 2:
            return None, None

        # Build normal equation components manually:
        # (A^T A) * p = A^T b

        Sxx = Sxz = Szz = 0.0
        Sxb = Szb = 0.0

        for x, z, yaw in self.throws:
            dx, dz = self._yaw_to_direction(yaw)

            # perpendicular normal
            nx = -dz
            nz = dx

            b = nx * x + nz * z

            Sxx += nx * nx
            Sxz += nx * nz
            Szz += nz * nz

            Sxb += nx * b
            Szb += nz * b

        est_x, est_z = self._solve_2x2(Sxx, Sxz, Sxz, Szz, Sxb, Szb)

        if est_x is None or est_z is None:
            return (0, 0), 0

        # acc
        total_error = 0.0
        for x, z, yaw in self.throws:
            dx, dz = self._yaw_to_direction(yaw)
            vx = est_x - x
            vz = est_z - z

            error = abs(vx * dz - vz * dx)
            total_error += error

        avg_error = total_error / len(self.throws)

        return (est_x, est_z), avg_error


if __name__ == "__main__":
    c1 = '/execute in minecraft:overworld run tp @s 1015.62 71.00 -1288.43 -989.42 -28.83'
    c2 = '/execute in minecraft:overworld run tp @s 503.94 63.00 -1344.39 -632.43 -28.46'

    triangulator = StrongholdTriangulator()
    triangulator.insert_f3c_string(c1)
    triangulator.insert_f3c_string(c2)

    coords, acc = triangulator.calculate()
    print(f"Estimated Stronghold: {coords}, Avg Error: {acc:.2f} blocks")