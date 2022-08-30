const { testZscalerIntegration } = require('./test-integration');

async function test () {
  let statusCodes;

  try {
    statusCodes = await testZscalerIntegration(true);

    statusCodes.forEach((statusCode) => {
      if (statusCode !== 200) {
        process.exit(1);
      }
    });

    process.exit(0);
  } catch (error) {
    console.error(error);
    process.exit(1);
  };
}

test();
